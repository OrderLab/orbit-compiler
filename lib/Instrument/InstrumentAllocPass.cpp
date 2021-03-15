// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "Instrument/InstrumentAllocPass.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "instrument-alloc"

using namespace std;
using namespace llvm;
using namespace llvm::instrument;

void InstrumentAllocPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool InstrumentAllocPass::runOnModule(Module &M) {
  bool modified = false;
  _instrumenter = make_unique<AllocInstrumenter>();
  LLVMContext &context = M.getContext();
  if (!_instrumenter->initHookFuncs(&M, context)) {
    return false;
  }
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function &F = *I;
    if (!F.isDeclaration()) {
      modified |= runOnFunction(F);
    }
  }
  errs() << "Instrumented " << _instrumenter->getInstrumentedCnt()
         << " instructions in total\n";
  return modified;
}

bool InstrumentAllocPass::runOnFunction(Function &F) {
  bool instrumented = false;
  Instruction *instr;
  for (inst_iterator ii = inst_begin(F), ie = inst_end(F); ii != ie; ++ii) {
    instr = &*ii;
    instrumented |= _instrumenter->instrumentInstr(instr);
  }
  return instrumented;
}

char InstrumentAllocPass::ID = 2;
static RegisterPass<InstrumentAllocPass> X(
    "instm", "Instruments allocation related code");
