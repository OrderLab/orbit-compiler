// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _OBIWANANALYSIS_H_
#define _OBIWANANALYSIS_H_

#include "CallGraph/CallGraph.h"
#include "DefUse/DefUse.h"
#include "Utils/LLVM.h"

#include "llvm/IR/Value.h"

using namespace llvm;
using namespace defuse;

class ObiWanAnalysis {
 private:
  CallGraph callGraph;
  Value *root;      // llvm Value to track
  Function *start;  // heap allocation's parent function
  Function *end;    // target function (eg: check_and_resolve)
  UserGraph ug;
  Optional<bool> calcIsAllocationPoint;

 public:
  ObiWanAnalysis(Value *root, Function *start, Function *end,
      const std::set<std::string> &alloc_funcs);
  ~ObiWanAnalysis() {};
  bool isAllocationPoint();
  void performDefUse();
};

#endif  // _OBIWANANALYSIS_H_
