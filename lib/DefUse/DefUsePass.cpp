//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//
//  Simple def-use LLVM pass
//
//  Author: Ryan Huang <huang@cs.jhu.edu>
//

#include "DefUse/DefUsePass.h"
#include "DefUse/DefUse.h"

#include "llvm/Support/CommandLine.h"

using namespace std;
using namespace llvm;
using namespace llvm::defuse;

// opt argument to analyze def-use only in the target functions
static cl::list<std::string> TargetFunctions("target-functions",
                                             cl::desc("<Function>"),
                                             cl::ZeroOrMore);

char MyDefUsePass::ID = 0;
static RegisterPass<MyDefUsePass> X(
    "mydefuse",
    "Pass that analyzes definition-usage chain of program variables");

bool MyDefUsePass::runOnModule(Module &M) {
  set<string> targetFunctionSet(TargetFunctions.begin(), TargetFunctions.end());
  bool modified = false;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function &F = *I;
    if (!F.isDeclaration()) {
      if (targetFunctionSet.empty() ||
          targetFunctionSet.count(F.getName()) != 0)
        modified |= runOnFunction(F);
    }
  }
  return modified;
}

bool MyDefUsePass::runOnFunction(Function &F) {
  if (F.isDeclaration()) {
    errs() << "This is a declaration so we do not do anything\n";
    return false;
  }
  dbgs() << "[" << F.getName() << "]\n";
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    Instruction *inst = &*I;
    errs() << "I: " << *inst << "\n";
  }
  return false;
}
