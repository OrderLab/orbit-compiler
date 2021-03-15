//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "Utils/LLVM.h"

#ifndef __CALLGRAPH_H_
#define __CALLGRAPH_H_

using namespace llvm;

enum class EdgeType { CALL, RETURN };

struct Edge {
 public:
  Function *dst;
  EdgeType edgeType;

  Edge(Function *f, EdgeType type) {
    dst = f;
    edgeType = type;
  }
};

// Basic Graph class for interprocedural Call Graph generation in LLVM
// The graph is directed
class CallGraph {
  typedef std::unordered_map<Function *, std::set<Edge *>> Graph;
  typedef std::vector<Function *> Path;
  typedef std::set<Function *> Visited;
  typedef std::queue<Function *> VisitQueue;

 public:
  CallGraph();

  void addEdge(Function *caller, Function *callee);
  void printPath(Function *source, Function *destination);

 private:
  Path findPath(Function *source, Function *destination);

 private:
  Graph callGraph;
};

#endif  //__CALLGRAPH_H_