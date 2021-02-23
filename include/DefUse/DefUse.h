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

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace defuse {

enum class UserGraphWalkType { BFS, DFS };

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

  UserGraph(Value *v, UserGraphWalkType t = UserGraphWalkType::DFS,
            int depth = -1) {
    root = v;
    maxDepth = depth;
    if (t == UserGraphWalkType::DFS)
      doDFS();
    else
      doBFS();
  }

  const_iterator begin() const { return userList.begin(); }
  const_iterator end() const { return userList.end(); }
  iterator begin() { return userList.begin(); }
  iterator end() { return userList.end(); }

 protected:
  void doDFS();
  void doBFS();

 private:
  void processUser(Value *elem, UserGraphWalkType walk);

 private:
  int maxDepth;
  Value *root;
  UserNodeList userList;
  VistedNodeSet visited;
  VisitQueue visit_queue;
  VisitStack visit_stack;
};

}  // namespace defuse
}  // namespace llvm

#endif /* __DEFUSE_H_ */
