// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "ObiWanAnalysis/ObiWanAnalysis.h"

ObiWanAnalysis::ObiWanAnalysis(Value *root, Function *start, Function *end,
    const AllocRules &rules)
  : callGraph(), root(root), start(start), end(end),
    ug(root, &callGraph, end, rules),
    calcIsAllocationPoint()
{}

void ObiWanAnalysis::performDefUse() {
  calcIsAllocationPoint = ug.run(UserGraphWalkType::BFS);
  if (!calcIsAllocationPoint.getValue()) return;

  printCallSite(root);
  // callGraph.printPath(start, end);
  errs() << '\n';
}

bool ObiWanAnalysis::isAllocationPoint() {
  return calcIsAllocationPoint.getValueOr(false);
}
