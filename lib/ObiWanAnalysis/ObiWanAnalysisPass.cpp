// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "DefUse/DefUse.h"
#include "Instrument/AllocInstrumenter.h"
#include "ObiWanAnalysis/ObiWanAnalysis.h"
#include "Utils/LLVM.h"

#include "llvm/ADT/MapVector.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::defuse;
using namespace llvm::instrument;

static cl::list<std::string> TargetFunctions("target-functions",
                                             cl::desc("<Function>"),
                                             cl::ZeroOrMore);

struct ObiWanAnalysisPass : public llvm::ModulePass {
  static char ID;

  const AllocRules rules = {
    .alloc{
      // standard
      "malloc", "calloc",
      // MySQL
      "mem_heap_alloc", "mem_heap_zalloc",
      "mem_strdup", "mem_strdupl", "mem_heap_strdupl",
      "ut_allocator<unsigned char>::allocate",
      // Redis
      "zmalloc", "zcalloc", "zstrdup",
      // Nginx
      "ngx_alloc", "ngx_calloc", "ngx_memalign",
      "ngx_palloc", "ngx_pcalloc", "ngx_pnalloc", "ngx_pmemalign", "ngx_palloc_large",
      // Apache
      "apr_pcalloc", "apr_palloc",
    },
    .dealloc{
      // standard
      {"free", 0},
      // MySQL
      {"ut_allocator<unsigned char>::deallocate", 1}, {"ut_allocator<unsigned char>::reallocate", 1},
      // Redis
      {"zfree", 0},
      // Nginx
      {"ngx_free", 0}, {"ngx_pfree", 1},
    },
    .realloc{
      // standard
      {"realloc", 0},
      // Redis
      {"zrealloc", 0},
    },
    .ignored{
      // MySQL
      "mem_heap_*", "ut_allocator*",
      // Redis
      "zmalloc_*", "je_*",
      // Nginx
      "ngx_pool_*", "ngx_palloc_*", "ngx_create_pool", "ngx_destroy_pool", "ngx_reset_pool",
    }
  };

  std::vector<Instruction *> heapCalls;

  ObiWanAnalysisPass() : llvm::ModulePass(ID) {}
  // Pool<trx_t, TrxFactory, TrxPoolLock>::Pool

  bool runOnModule(Module &M) override {
    bool modified = false;
    addFunctionAttributes(M);

    // Find Allocation Points
    std::set<std::string> targetFunctionSet(TargetFunctions.begin(),
                                            TargetFunctions.end());

    for (auto it = targetFunctionSet.begin(); it != targetFunctionSet.end();
         it++) {
      // Many functions with same name may exist. This is a gross
      // simplification
      Function *targetFun = getFunctionWithName(*it, M);

      if (targetFun == NULL) {
        dbgs() << "Could not find target function " << *it << "\n";
        continue;
      }

      for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
        Function &F = *I;
        if (!F.isDeclaration() &&
            rules.alloc.find(demangleName(F.getName())) == rules.alloc.end())
        {
          modified |= identifyHeapAlloc(F, targetFun, rules);
        }
      }
    }

    errs() << "Found heapCalls " << heapCalls.size() << "\n";

    return false;

    if (heapCalls.size() == 0) return false;

    // Perform Instrumentation
    AllocInstrumenter instrumenter(false);
    if (!instrumenter.initHookFuncs(&M, M.getContext())) {
      errs() << "Failed to initialize hook functions\n";
      return false;
    }

    for (auto heapCall : heapCalls) {
      modified |= instrumentInstruction(&instrumenter, heapCall);
    }

    errs() << "Instrumented " << instrumenter.getInstrumentedCnt()
           << " instructions in total\n";

    // Save Instrumented File
    std::string inputFileBasenameNoExt = "test";
    std::string outputFilename = inputFileBasenameNoExt + "-instrumented.bc";

    if (!saveModule(&M, outputFilename)) {
      errs() << "Failed to save the instrumented bitcode file to "
             << outputFilename << "\n";
      return modified;
    }

    errs() << "Successfully saved instrumented file " << outputFilename << "\n";
    return modified;
  }

  bool instrumentInstruction(AllocInstrumenter *instrumenter,
                             Instruction *inst) {
    return instrumenter->instrumentInstr(inst);
  }

  bool identifyHeapAlloc(Function &F, Function *targetFun, const AllocRules &rules) {
    if (F.isDeclaration() || F.isIntrinsic()) return false;
    std::string demangled = demangleFunctionName(&F);

    // if (demangled != "quicklistCreate") return false;

    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
      Instruction *inst = &*I;
      // Create CallSite object which is a common abstraction over call and
      // invoke instructions
      CallSite cs(inst);
      if (cs.isCall() || cs.isInvoke()) {
        Function *calledFun = cs.getCalledFunction();
        if (calledFun == NULL) continue;
        std::string calledName = demangleName(calledFun->getName());
        if (rules.alloc.find(calledName) != rules.alloc.end()) {
          ObiWanAnalysis ob(inst, &F, targetFun, rules);
          ob.performDefUse();
          if (ob.isAllocationPoint()) {
            heapCalls.push_back(inst);
          }
        }
      }
    }
    return false;
  }

  bool saveModule(Module *M, std::string outFile) {
    if (verifyModule(*M, &errs())) {
      errs() << "Error: module failed verification.\n";
      return false;
    }
    std::ofstream ofs(outFile);
    raw_os_ostream ostream(ofs);
    WriteBitcodeToFile(M, ostream);
    return true;
  }

  // Adds function attributes - currently not used
  // Reference:
  // http://web.archive.org/web/20160306050809/http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
  void addFunctionAttributes(Module &M) {
    auto global_annos = M.getNamedGlobal("llvm.global.annotations");
    if (global_annos) {
      auto a = cast<ConstantArray>(global_annos->getOperand(0));
      for (unsigned i = 0; i < a->getNumOperands(); i++) {
        auto e = cast<ConstantStruct>(a->getOperand(i));

        if (auto fn = dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
          auto anno = cast<ConstantDataArray>(
                          cast<GlobalVariable>(e->getOperand(1)->getOperand(0))
                              ->getOperand(0))
                          ->getAsCString();
          fn->addFnAttr(anno);
        }
      }
    }
  }
};

char ObiWanAnalysisPass::ID = 1;
RegisterPass<ObiWanAnalysisPass> X(
    "obi-wan-analysis", "Analysis to find allocation points for heap variables",
    true, true);
