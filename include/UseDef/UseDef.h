// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef __USEDEF_H_
#define __USEDEF_H_

#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include "set"

namespace llvm {
namespace usedef {

// Use Def Chain for a given Value (Instruction). Finds all the Def's for a
// given Use without any intervening Def's.
class UseDefChain {
 public:
  Value *root;
  std::set<CallInst *> heapCalls;

 private:
  void useDefAnalysis(Value *);
  void globalVariableAnalysis(GlobalValue *);

 public:
  UseDefChain(Value *val) {
    root = val;
    if (GlobalValue *global = dyn_cast<GlobalValue>(val)) {
      globalVariableAnalysis(global);
    } else {
      useDefAnalysis(root);
    }
  }
};

}  // namespace usedef
}  // namespace llvm

#endif /* __USEDEF_H_ */
