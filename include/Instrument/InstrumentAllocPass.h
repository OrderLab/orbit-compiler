// The Obi-wan Project
//
// Created by ryanhuang on 12/24/19.
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _INSTRUMENT_ALLOC_PASS_H_
#define _INSTRUMENT_ALLOC_PASS_H_

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include "Instrument/AllocInstrumenter.h"

namespace llvm {

class InstrumentAllocPass: public ModulePass {
 public:
  static char ID;
  InstrumentAllocPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

 protected:
  bool runOnFunction(Function &F);

 protected:
  std::unique_ptr<instrument::AllocInstrumenter> _instrumenter;
};

} // end of namespace llvm

#endif /* _INSTRUMENT_ALLOC_PASS_H_ */
