// The Obi-wan Project
//
// Copyright (c) 2020, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef __DEFUSE_H_
#define __DEFUSE_H_

#include <queue>
#include <stack>
#include <utility>
#include <vector>

#include "CallGraph/CallGraph.h"
#include "Utils/LLVM.h"

#include "llvm/IR/Argument.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Value.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/MemoryLocation.h"

#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace defuse {

enum class UserGraphWalkType { BFS, DFS };

// comp = [](const GetElementPtrInst &i1, const GetElementPtrInst &i2) {
//   return isAccessingSameStructVar(&i1, &i2);
// };

// User graph for a given instruction: it includes the direct users as well as
// the transitive closure of users' users etc.
class UserGraph {
 public:
  typedef std::pair<Value *, int> UserNode;
  typedef std::vector<UserNode> UserNodeList;
  typedef UserNodeList::iterator iterator;
  typedef UserNodeList::const_iterator const_iterator;
  typedef SmallPtrSet<Value *, 8> VistedNodeSet;
  typedef std::queue<Optional<Value *>> VisitQueue;
  typedef std::stack<Value *> VisitStack;
  typedef std::unordered_map<Function *, std::set<Type *>> FunctionSet;
  typedef std::unordered_map<Instruction *, std::vector<Instruction *>>
      ReturnCallMap;

  UserGraph(Value *v, CallGraph *cg, Function *targ, bool memberAnalysis,
            UserGraphWalkType t = UserGraphWalkType::DFS, int depth = -1) {
    root = v;
    maxDepth = depth;
    callGraph = cg;
    target = targ;
    this->memberAnalysis = memberAnalysis;
    if (t == UserGraphWalkType::DFS)
      doDFS();
    else
      doBFS();
  }

  const_iterator begin() const { return userList.begin(); }
  const_iterator end() const { return userList.end(); }
  iterator begin() { return userList.begin(); }
  iterator end() { return userList.end(); }

  FunctionSet::iterator func_begin() { return functionsVisited.begin(); }
  FunctionSet::iterator func_end() { return functionsVisited.end(); }

  bool isFunctionVisited(Function *);
  void printCallSite();

 protected:
  void doDFS();
  void doBFS();

 private:
  bool isIncompatibleFun(Function *fun);
  void processUser(Value *elem, UserGraphWalkType walk);
  void processGetElementPtr(Value *elem, UserGraphWalkType walk);
  void insertElement(Value *elem, UserGraphWalkType walk);
  void insertElementUsers(Value *elem, UserGraphWalkType walk);

 public:
  FunctionSet functionsVisited;
  std::vector<Function *> funVector;

 private:
  int maxDepth;
  Value *root;
  UserNodeList userList;
  VistedNodeSet visited;
  VisitQueue visit_queue;
  VisitStack visit_stack;
  CallGraph *callGraph;
  Function *target;
  ReturnCallMap returnCallMap;
  bool memberAnalysis;
};

}  // namespace defuse
}  // namespace llvm

#endif /* __DEFUSE_H_ */
