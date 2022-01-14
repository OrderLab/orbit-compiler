//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "CallGraph/CallGraph.h"
#include "llvm/Support/raw_ostream.h"

CallGraph::CallGraph() {}

void CallGraph::addEdge(Function *caller, Function *callee) {
  // Caller will have a directed edge to callee with CALL as edge type
  struct Edge *callEdge = new Edge(callee, EdgeType::CALL);
  callGraph[caller].insert(callEdge);

  // Callee will have a directed edge to caller with RETURN as edge type
  struct Edge *returnEdge = new Edge(caller, EdgeType::RETURN);
  callGraph[callee].insert(returnEdge);
}

void CallGraph::printPath(Function *source, Function *destination) {
  Path path = findPath(source, destination);
  if (path.size() == 0) {
    errs() << "No path found\n";
  } else {
    for (auto it = path.begin(); it != path.end(); it++) {
      errs() << demangleName((*it)->getName()) << "->";
    }
    errs() << "\n";
  }
}

CallGraph::Path CallGraph::findPath(Function *source, Function *destination) {
  Visited visited;
  CallGraph::Path path;
  std::unordered_map<Function *, Function *> prev;
  std::set<Function *> calledFuns;
  std::queue<Function *> queue;
  bool found = false;

  queue.push(source);
  prev[source] = NULL;

  while (!queue.empty()) {
    Function *front = queue.front();
    queue.pop();
    visited.insert(front);

    for (auto edge : callGraph[front]) {
      if (visited.count(edge->dst) == 0) {
        if (edge->edgeType == EdgeType::CALL) {
          queue.push(edge->dst);
          prev[edge->dst] = front;
          calledFuns.insert(edge->dst);
        } else {
          if (calledFuns.count(front) != 0) {
            // Ignore: Can only return to the original function
            // which we have already done
            continue;
          }
          queue.push(edge->dst);
          prev[edge->dst] = front;
        }
        if (edge->dst == destination) {
          found = true;
          break;
        }
      }
    }
    if (found) break;
  }

  if (!found) return path;

  // Construct Path
  Function *curr = destination;
  while (curr != NULL && curr != prev[curr]) {
    path.push_back(curr);
    curr = prev[curr];
  }

  return path;
}
