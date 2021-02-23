// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//
//  Simple def-use graph
//
//  Author: Ryan Huang <huang@cs.jhu.edu>
//

#include "DefUse/DefUse.h"

using namespace std;
using namespace llvm;
using namespace llvm::defuse;

void UserGraph::doDFS() {
  visited.insert(root);
  visit_stack.push(root);
  while (!visit_stack.empty()) {
    Value *elem = visit_stack.top();
    if (elem != root && !isa<AllocaInst>(elem)) {
      // skip alloca inst, which comes from the operand of StoreInstruction.
      userList.push_back(UserNode(elem, 1));
    }
    visit_stack.pop();
    processUser(elem, UserGraphWalkType::DFS);
  }
}

void UserGraph::doBFS() {
  visited.insert(root);
  visit_queue.push(root);
  visit_queue.push(None);  // a dummy element marking end of a level
  int level = 0;
  while (!visit_queue.empty()) {
    Optional<Value *> head = visit_queue.front();
    if (head != None) {
      Value *elem = head.getValue();
      if (elem != root && !isa<AllocaInst>(elem)) {
        // skip alloca inst, which comes from the operand of StoreInstruction.
        userList.push_back(UserNode(elem, level));
      }
      processUser(elem, UserGraphWalkType::BFS);
    }
    visit_queue.pop();  // remove the front element
    if (!visit_queue.empty()) {
      head = visit_queue.front();
      if (head != None) {
        continue;
      }
      // reached the marker, meaning we are about to go to the next level
      // check the level depth here so we may stop
      level += 1;
      visit_queue.pop();
      if (maxDepth > 0 && level > maxDepth) {
        break;
      }
      if (!visit_queue.empty()) {
        // if this is not the last, add a marker at the end of the visit_queue
        visit_queue.push(None);
      }
    }
  }
}

void UserGraph::processUser(Value *elem, UserGraphWalkType walk) {
  for (auto ui = elem->user_begin(); ui != elem->user_end(); ++ui) {
    if (Instruction *inst = dyn_cast<Instruction>(*ui)) {
      if (StoreInst *store = dyn_cast<StoreInst>(inst)) {
        if (Instruction *op = dyn_cast<Instruction>(store->getOperand(1))) {
          // if this is a store instruction, it does not have users. we
          // need to find the users of the target (second operand) instead.
          if (visited.insert(op).second) {
            if (walk == UserGraphWalkType::DFS)
              visit_stack.push(op);
            else
              visit_queue.push(op);
          }
        }
      } else {
        if (visited.insert(inst).second) {
          if (walk == UserGraphWalkType::DFS)
            visit_stack.push(inst);
          else
            visit_queue.push(inst);
        }
      }
    }
  }
}
