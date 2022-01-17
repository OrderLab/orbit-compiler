// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "ObiWanAnalysis/ObiWanAnalysis.h"

ObiWanAnalysis::ObiWanAnalysis(Value *root, Function *start, Function *end, Module &M)
  : callGraph(), root(root), start(start), end(end),
    ug(root, &callGraph, end, isConstructor(root), M), calcIsAllocationPoint()
{}

void ObiWanAnalysis::performDefUse() {
  ug.run(UserGraphWalkType::BFS);

  if (!isAllocationPoint()) return;
  printCallSite(root);
  callGraph.printPath(start, end);
  errs() << '\n';
}

bool ObiWanAnalysis::isAllocationPoint() {
  if (!calcIsAllocationPoint.hasValue())
    calcIsAllocationPoint = ug.isFunctionVisited(end) && ug.functionGlobalVarVisited(end);
  return calcIsAllocationPoint.getValue();
}
