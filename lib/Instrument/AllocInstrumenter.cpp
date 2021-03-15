// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "Instrument/AllocInstrumenter.h"

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugLoc.h>

#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>

#define DEBUG_TYPE "alloc-instrumenter"

using namespace std;
using namespace llvm;
using namespace llvm::slicing;
using namespace llvm::instrument;

unsigned int llvm::instrument::AllocVarGuidStart = 200;
static unsigned int AllocVarCurrentGuid = llvm::instrument::AllocVarGuidStart;

bool AllocInstrumenter::initHookFuncs(Module *M, LLVMContext &llvm_context) {
  if (_initialized) {
    errs() << "already initialized\n";
    return true;
  }
  _main = M->getFunction("main");
  if (!_main) {
    errs() << "Failed to find main function\n";
    return false;
  }

  auto I1Ty = Type::getInt1Ty(llvm_context);
  auto VoidTy = Type::getVoidTy(llvm_context);

  _I32Ty = Type::getInt32Ty(llvm_context);

  // need i8* for later orbit_track_gobj call
  _I8PtrTy = Type::getInt8PtrTy(llvm_context);

  if (!_track_with_printf) {
    // Add the external tracker function declarations.
    //
    // Here we only need to add the declarations; the definitions of these
    // functions will be provided when linking the instrumented bitcode
    // with the address tracker runtime library.
    //
    // The declaration must match the function signatures defined in
    // runtime/gobj_tracker.h

    StringRef funcName = getRuntimeHookInitName();
    _tracker_init_func =
        cast<Function>(M->getOrInsertFunction(funcName, VoidTy));
    if (!_tracker_init_func) {
      errs() << "could not find function " << funcName << "\n";
      return false;
    } else {
      DEBUG(dbgs() << "found tracker initialization function " << funcName
                   << "\n");
    }

    _track_gobj_func = cast<Function>(
        M->getOrInsertFunction(getRuntimeHookName(), _I8PtrTy, _I32Ty));
    if (!_track_gobj_func) {
      errs() << "could not find function " << getRuntimeHookName() << "\n";
      return false;
    } else {
      DEBUG(dbgs() << "found track gobj function " << getRuntimeHookName()
                   << "\n");
    }

    _tracker_dump_func =
        cast<Function>(M->getOrInsertFunction(getTrackDumpHookName(), I1Ty));
    if (!_tracker_dump_func) {
      errs() << "could not find function " << getTrackDumpHookName() << "\n";
      return false;
    } else {
      DEBUG(dbgs() << "found track dump function " << getTrackDumpHookName()
                   << "\n");
    }

    _tracker_finish_func = cast<Function>(
        M->getOrInsertFunction(getTrackHookFinishName(), VoidTy));
    if (!_tracker_finish_func) {
      errs() << "could not find function " << getTrackHookFinishName() << "\n";
      return false;
    } else {
      DEBUG(dbgs() << "found track finish function " << getTrackHookFinishName()
                   << "\n");
    }
  }

  // get or create printf function declaration:
  //    int printf(const char * format, ...);
  vector<Type *> printfArgsTypes;
  printfArgsTypes.push_back(_I8PtrTy);
  FunctionType *printfType = FunctionType::get(_I32Ty, printfArgsTypes, true);
  _printf_func = cast<Function>(M->getOrInsertFunction("printf", printfType));
  if (!_printf_func) {
    errs() << "could not find printf\n";
    return false;
  } else {
    DEBUG(dbgs() << "found printf\n");
  }

  // Only insert the tracker init and finish function if we choose to instrument
  // using the runtime library. For printf-based tracking, it is not needed.
  if (!_track_with_printf) {
    // insert call to __orbit_gobj_tracker_init at the beginning of main
    // function
    IRBuilder<> builder(
        cast<Instruction>(_main->front().getFirstInsertionPt()));
    builder.CreateCall(_tracker_init_func);
    errs() << "Instrumented call to " << getRuntimeHookInitName()
           << " in main\n";

    // insert call to __orbit_gobj_tracker_finish at program exit
    appendToGlobalDtors(*M, _tracker_finish_func, 1);
    errs() << "Instrumented call to " << getTrackHookFinishName()
           << " in main\n";
  }

  _initialized = true;
  return true;
}

bool AllocInstrumenter::instrumentInstr(Instruction *instr) {
  // first check if this instruction has been instrumented before
  if (_hook_point_guid_map.find(instr) != _hook_point_guid_map.end()) {
    DEBUG(errs() << "Skip instrumenting instruction (" << *instr
                 << ") as it has been instrumented before\n");
    return false;
  }

  Value *alloc_size;
  Value *alloc_addr;

  CallSite cs(instr);
  if (!cs.isInvoke() && !cs.isCall()) return false;
  Function *callee = cs.getCalledFunction();
  if (callee == NULL) return false;

  alloc_addr = instr;
  std::string demangled = demangleFunctionName(callee);
  errs() << "Got " << demangled << "\n";

  if (demangled.compare("mem_heap_alloc") == 0) {
    alloc_size = cs.getArgument(1);
  } else if (demangled.compare("ut_allocator<unsigned char>::allocate") == 0) {
    alloc_size = cs.getArgOperand(1);
  } else {
    return false;
  }

  // insert a call instruction to the hook function with CallInst::Create.
  // Pass addr as an argument to this call instruction
  IRBuilder<> builder(instr);
  builder.SetInsertPoint(instr);

  if (_track_with_printf) {
    Value *str = builder.CreateGlobalStringPtr("orbit alloc: %zu => %p\n");
    std::vector<llvm::Value *> params;
    params.push_back(str);
    params.push_back(alloc_size);
    params.push_back(alloc_addr);
    builder.CreateCall(_printf_func, params);
  } else {
    // Replace callInst with an __orbit_track_gobj call
    // We use ReplaceInstWithInst
    // need to explicitly cast the address, which could be i32* or i64*, to i8*
    _hook_point_guid_map[instr] = AllocVarCurrentGuid;
    _guid_hook_point_map[AllocVarCurrentGuid] = instr;
    std::vector<llvm::Value *> args;
    auto i32size = builder.CreateIntCast(alloc_size, _I32Ty, false);
    args.push_back(i32size);
    Instruction *newInstr = NULL;
    if (isa<CallInst>(instr)) {
      newInstr = CallInst::Create(_track_gobj_func, args);
    } else if (isa<InvokeInst>(instr)) {
      InvokeInst *ii = dyn_cast<InvokeInst>(instr);
      newInstr = InvokeInst::Create(_track_gobj_func, ii->getNormalDest(),
                                    ii->getUnwindDest(), args);
    } else {
      return false;
    }
    ReplaceInstWithInst(instr, newInstr);

    AllocVarCurrentGuid++;
  }
  _instrument_cnt++;
  return true;
}