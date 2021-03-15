// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "ObiWanAnalysis/ObiWanAnalysis.h"

ObiWanAnalysis::ObiWanAnalysis(Value *root, Function *start, Function *end) {
  this->start = start;
  this->end = end;
  this->root = root;
  this->callGraph = new CallGraph();
}

void ObiWanAnalysis::performDefUse() {
  if (isConstructor(root)) {
    ug = new UserGraph(root, callGraph, end, true, UserGraphWalkType::BFS);
  } else {
    ug = new UserGraph(root, callGraph, end, false, UserGraphWalkType::BFS);
  }
  if (!ug->isFunctionVisited(end)) return;
  printCallSite(root);
  callGraph->printPath(start, end);
}

bool ObiWanAnalysis::isAllocationPoint() { return ug->isFunctionVisited(end); }