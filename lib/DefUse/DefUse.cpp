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

bool UserGraph::isFunctionVisited(Function *fun) {
  if (functionsVisited.count(fun) != 0) return true;
  return false;
}

void UserGraph::doDFS() {
  visited.insert(root);
  visit_stack.push(root);
  while (!visit_stack.empty()) {
    if (isFunctionVisited(target)) return;
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
    if (isFunctionVisited(target)) return;
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
  if (Instruction *ins = dyn_cast<Instruction>(elem)) {
    Function *fun = ins->getFunction();
    Type *type = ins->getType();
    functionsVisited[fun].insert(type);
  }

  // Process Value
  if (StoreInst *store = dyn_cast<StoreInst>(elem)) {
    insertElement(store->getOperand(1), walk);
  }

  if (ReturnInst *ret = dyn_cast<ReturnInst>(elem)) {
    if (returnCallMap.count(ret) != 0) {
      for (auto inst : returnCallMap[ret]) {
        // We have already inserted the call/Invoke Instruction, we need to
        // insert it's users when returning
        insertElementUsers(inst, walk);
      }
    } else {
      Function *fun = ret->getFunction();
      for (auto user : fun->users()) {
        if (Instruction *ins = dyn_cast<Instruction>(user)) {
          if (isIncompatibleFun(ins->getFunction())) continue;
          callGraph->addEdge(ret->getFunction(), ins->getFunction());
        }
        insertElement(user, walk);
      }
    }
  }
  if (memberAnalysis) {
    processGetElementPtr(elem, walk);
  }

  // Process Value users
  for (auto ui : elem->users()) {
    if (CallInst *callInst = dyn_cast<CallInst>(ui)) {
      // Call Instruction - need to save argument if one exists

      // Edge Case: If there are no operands then we are not using elem in the
      // call instruction We can directly add callInst to the queue/stack since
      // we are still interested in the return value
      if (callInst->getNumArgOperands() == 0) {
        Function *caller = callInst->getFunction();
        Function *callee = callInst->getCalledFunction();
        callGraph->addEdge(caller, callee);
        insertElement(callInst, walk);
        continue;
      }
      unsigned int arg_no;
      for (arg_no = 0; arg_no < callInst->getNumArgOperands(); arg_no++) {
        if (callInst->getArgOperand(arg_no) == elem) break;
      }
      Function *caller = callInst->getFunction();
      Function *callee = callInst->getCalledFunction();
      if (isIncompatibleFun(callee) || callee->isVarArg()) return;
      callGraph->addEdge(caller, callee);
      Argument *arg = callee->arg_begin() + arg_no;
      insertElement(arg, walk);

      for (inst_iterator I = inst_begin(callee), E = inst_end(callee); I != E;
           ++I) {
        Instruction *inst = &*I;
        if (ReturnInst *returnInst = dyn_cast<ReturnInst>(inst)) {
          returnCallMap[returnInst].push_back(callInst);
        }
      }
    }
    if (InvokeInst *invokeInst = dyn_cast<InvokeInst>(ui)) {
      // Invoke Instruction - need to save argument if one exists

      // Edge Case: If there are no operands then we are not using elem in the
      // call instruction We can directly add invokeInst to the queue/stack
      // since we are still interested in the return value
      if (invokeInst->getNumArgOperands() == 0) {
        Function *caller = invokeInst->getFunction();
        Function *callee = invokeInst->getCalledFunction();
        callGraph->addEdge(caller, callee);
        insertElement(invokeInst, walk);
        continue;
      }

      unsigned int arg_no;
      for (arg_no = 0; arg_no < invokeInst->getNumArgOperands(); arg_no++) {
        if (invokeInst->getArgOperand(arg_no) == elem) break;
      }
      Function *caller = invokeInst->getFunction();
      Function *callee = invokeInst->getCalledFunction();
      if (isIncompatibleFun(callee) || callee->isVarArg()) return;
      callGraph->addEdge(caller, callee);
      Argument *arg = callee->arg_begin() + arg_no;
      insertElement(arg, walk);

      for (inst_iterator I = inst_begin(callee), E = inst_end(callee); I != E;
           ++I) {
        Instruction *inst = &*I;
        if (ReturnInst *returnInst = dyn_cast<ReturnInst>(inst)) {
          returnCallMap[returnInst].push_back(invokeInst);
        }
      }
    } else {
      insertElement(ui, walk);
    }
  }
}

void UserGraph::processGetElementPtr(Value *elem, UserGraphWalkType walk) {
  if (GetElementPtrInst *getInst1 = dyn_cast<GetElementPtrInst>(elem)) {
    Function *curr = getInst1->getFunction();
    std::string name = demangleName(curr->getName());
    if (curr->arg_size() == 0) return;
    if (name.rfind("Pool", 0) == 0 || name.rfind("ha_innobase", 0) == 0 ||
        name.rfind("create_table_info_t", 0) == 0) {
      Argument *arg = curr->arg_begin();
      Module *mod = curr->getParent();
      auto funs = getFunctionsWithType(arg->getType(), *mod);
      for (auto fun : funs) {
        for (inst_iterator I = inst_begin(fun), E = inst_end(fun); I != E;
             ++I) {
          Instruction *inst = &*I;
          if (GetElementPtrInst *getInst2 = dyn_cast<GetElementPtrInst>(inst)) {
            if (isAccessingSameStructVar(getInst1, getInst2)) {
              if (isIncompatibleFun(getInst2->getFunction())) return;
              callGraph->addEdge(getInst1->getFunction(),
                                 getInst2->getFunction());
              insertElement(getInst2, walk);
            }
          }
        }
      }
    }
  }
}

void UserGraph::insertElementUsers(Value *elem, UserGraphWalkType walk) {
  for (auto user : elem->users()) {
    insertElement(user, walk);
  }
}

void UserGraph::insertElement(Value *elem, UserGraphWalkType walk) {
  if (visited.insert(elem).second) {
    if (walk == UserGraphWalkType::DFS)
      visit_stack.push(elem);
    else
      visit_queue.push(elem);
  }
}

bool UserGraph::isIncompatibleFun(Function *fun) {
  if (fun == NULL || fun->isIntrinsic()) return true;
  std::string name = demangleName(fun->getName());
  if (name.rfind("std", 0) == 0 || name.rfind("boost", 0) == 0 ||
      name.rfind("ib::logger", 0) == 0 || name.rfind("PolicyMutex", 0) == 0 ||
      name.rfind("__gnu", 0) == 0 || name.rfind("ut_allocator", 0) == 0) {
    return true;
  }
  return false;
}