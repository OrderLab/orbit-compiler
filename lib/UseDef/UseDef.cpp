// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "UseDef/UseDef.h"

using namespace std;
using namespace llvm;
using namespace llvm::usedef;

void UseDefChain::useDefAnalysis(Value *value) {
  // Value is an Argument - find call sites
  if (Argument *arg = dyn_cast<Argument>(value)) {
    Function *function = arg->getParent();
    unsigned int arg_no = arg->getArgNo();
    for (auto user : function->users()) {
      CallInst *callIns = dyn_cast<CallInst>(user);
      useDefAnalysis(callIns->getOperand(arg_no));
    }
  }

  // Value is a Load Instruction
  if (LoadInst *loadIns = dyn_cast<LoadInst>(value)) {
    assert(loadIns->getNumOperands() == 1);
    useDefAnalysis(loadIns->getOperand(0));
  }

  // Value is an Allocation Instruction - get the users and find the store
  // instruction
  if (AllocaInst *allocIns = dyn_cast<AllocaInst>(value)) {
    for (auto it : allocIns->users()) {
      if (dyn_cast<StoreInst>(it)) {
        useDefAnalysis(it);
      }
    }
  }

  // Value is a Store Instruction - use the first operand
  if (StoreInst *storeIns = dyn_cast<StoreInst>(value)) {
    useDefAnalysis(storeIns->getOperand(0));
  }

  // Value is a Call Instruction - get the called function OR check if it's a
  // malloc
  if (CallInst *callIns = dyn_cast<CallInst>(value)) {
    std::string name = callIns->getCalledFunction()->getName().str();
    if (name.compare("malloc") == 0) {
      errs() << *callIns << "\n";
      heapCalls.insert(callIns);
    } else {
      useDefAnalysis(callIns->getCalledFunction());
    }
  }

  // BitCast Instruction
  if (BitCastInst *bitIns = dyn_cast<BitCastInst>(value)) {
    assert(bitIns->getNumOperands() == 1);
    useDefAnalysis(bitIns->getOperand(0));
  }

  // Value is a Function - need to loop through all BasicBlocks and examine each
  // return instruction
  if (Function *function = dyn_cast<Function>(value)) {
    for (auto &BB : *function) {
      for (auto &I : BB) {
        if (ReturnInst *returnIns = dyn_cast<ReturnInst>(&I)) {
          useDefAnalysis(returnIns->getReturnValue());
        }
      }
    }
  }
}

void UseDefChain::globalVariableAnalysis(GlobalValue *global) {
  errs() << "Global Variable";
  for (auto user: global->users()) {
    errs() << *user << "\n";
  }
}