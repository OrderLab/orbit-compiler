// The Obi-wan Project
//
// Copyright (c) 2020, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _DEF_USE_PASS_H_
#define _DEF_USE_PASS_H_

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {

class MyDefUsePass : public ModulePass {
 public:
  static char ID;
  MyDefUsePass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

 private:
  bool runOnFunction(Function &F);
};

}  // end of namespace llvm

#endif /* _DEF_USE_PASS_H_ */
