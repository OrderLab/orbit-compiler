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

#include <unordered_set>

#include "DefUse/DefUse.h"

using namespace std;
using namespace llvm;
using namespace llvm::defuse;

const bool DBG = false;

static bool functionGlobalVarVisitedHelper(
    Function *fun,
    std::unordered_set<Function*> &visited_fuctions,
    const UserGraph::VisitedNodeSet &visited_values)
{
  bool DBG = false;
  if (fun == nullptr) return false;
  if (visited_fuctions.find(fun) != visited_fuctions.end())
    return false;
  visited_fuctions.insert(fun);
  // FIXME: false data flow to global variables
  // FIXME: write rules for data flow into and out of allocator functions
  if (fun->getName().find("je_") == 0 ||
      fun->getName().find("llvm.") == 0)
    return false;
  /* if (fun->getName().find("rdbSaveRio") == 0)
    DBG = true; */
  if (DBG) errs() << " === begin global var check === : " << fun->getName() << '\n';

  for (inst_iterator I = inst_begin(fun), E = inst_end(fun); I != E; ++I) {
    Instruction *inst = &*I;

    if (LoadInst *load = dyn_cast<LoadInst>(inst)) {
      if (DBG) errs() << "    " << *inst << '\n';
      // Is a global variable, and is a pointer type
      const Value *op0 = load->getOperand(0);
      // const Type *op0ty = op0->getType();
      if (isa<GlobalVariable>(op0)) {
        if (// op0ty->isPointerTy() && op0ty->getContainedType(0)->isPointerTy() &&
          visited_values.find(op0) != visited_values.end())
        {
          if (DBG) errs() << "    global var name: " << op0->getName() << '\n';
          return true;
        }
      } else if (const ConstantExpr *expr = dyn_cast<ConstantExpr>(op0)) {
        // Getting a field of global variable
        if (expr->getOpcode() != Instruction::GetElementPtr) continue;
        Constant *op0op0 = expr->getOperand(0);
        if (isa<GlobalVariable>(op0op0) &&
          visited_values.find(op0op0) != visited_values.end()) {
          if (DBG) errs() << "    global var name: " << op0op0->getName() << '\n';
          return true;
        }
      }
    } else if (GetElementPtrInst *getelem = dyn_cast<GetElementPtrInst>(inst)) {
      // Is a global variable, and is a pointer type
      const Value *op0 = getelem->getOperand(0);
      // const Type *getelemty = getelem->getType();
      if (isa<GlobalVariable>(op0) &&
          // getelemty->isPointerTy() &&
          visited_values.find(op0) != visited_values.end())
      {
        if (DBG) errs() << "    global var name: " << op0->getName() << '\n';
        return true;
      }
    } else if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
      // For newer LLVm, cast to Callbase
      Function *callee = CallSite(inst).getCalledFunction();
      if (functionGlobalVarVisitedHelper(callee, visited_fuctions, visited_values))
        return true;
    }
  }
  if (DBG) errs() << " === end global var check === : " << fun->getName() << '\n';
  return false;
}

bool UserGraph::functionGlobalVarVisited(Function *fun) {
  if (DBG) errs() << "End function is : " << fun->getName() << '\n';
  std::unordered_set<Function*> visited_fuctions;
  return functionGlobalVarVisitedHelper(fun, visited_fuctions, visited);
}

bool UserGraph::isFunctionVisited(Function *fun) {
  return functionsVisited.count(fun) != 0;
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
  /* if (Instruction *ins = dyn_cast<Instruction>(root))
    errs() << " === root is in : " << ins->getFunction()->getName() << '\n'; */
  visit_queue.push(nullptr);  // a dummy element marking end of a level
  int level = 0;
  while (!visit_queue.empty()) {
    if (isFunctionVisited(target)) return;
    Value* head = visit_queue.front();
    if (head != nullptr) {
      if (head != root && !isa<AllocaInst>(head)) {
        // skip alloca inst, which comes from the operand of StoreInstruction.
        userList.push_back(UserNode(head, level));
      }
      processUser(head, UserGraphWalkType::BFS);
    }
    visit_queue.pop();  // remove the front element
    if (!visit_queue.empty()) {
      head = visit_queue.front();
      if (head != nullptr) {
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
        visit_queue.push(nullptr);
      }
    }
  }
}

void UserGraph::processUser(Value *elem, UserGraphWalkType walk) {
  // bool DBG = true;
  if (DBG) errs() << " === begin process user === : " << *elem << '\n';
  if (Instruction *ins = dyn_cast<Instruction>(elem)) {
    Function *fun = ins->getFunction();
    if (DBG) errs() << "    in func: " << fun->getName() << '\n';
    Type *type = ins->getType();
    functionsVisited[fun].insert(type);
  }

  // TODO: Move Constant handling from insert to here

  // Process Value
  if (StoreInst *store = dyn_cast<StoreInst>(elem)) {
    insertElement(store->getOperand(1), walk);
  }
  // FIXME: add pointer chain info
  if (GetElementPtrInst *getelem = dyn_cast<GetElementPtrInst>(elem)) {
    if (DBG) errs() << "    operand is: " << *getelem->getOperand(0) << '\n';

    if (LoadInst *load = dyn_cast<LoadInst>(getelem->getOperand(0))) {
      insertElement(load->getOperand(0), walk);
    } else {
      insertElement(getelem->getOperand(0), walk);
    }
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
  if (false || memberAnalysis) {
    processGetElementPtr(elem, walk);
  }

  // Process Value users
  for (auto ui : elem->users()) {
    Instruction *ii = dyn_cast<Instruction>(ui);
    // TODO: add arguments modification to data flow analysis
    if (isa<CallInst>(ui) || isa<InvokeInst>(ui)) {
      // For newer LLVM, cast to Callbase
      CallSite callInst = CallSite(ui);
      // Call Instruction - need to save argument if one exists

      // Edge Case: If there are no operands then we are not using elem in the
      // call instruction We can directly add callInst to the queue/stack since
      // we are still interested in the return value
      if (callInst.getNumArgOperands() == 0) {
        Function *caller = callInst->getFunction();
        Function *callee = callInst.getCalledFunction();
        callGraph->addEdge(caller, callee);
        insertElement(ui, walk);
        continue;
      }
      unsigned int arg_no;
      for (arg_no = 0; arg_no < callInst.getNumArgOperands(); arg_no++) {
        if (callInst.getArgOperand(arg_no) == elem) break;
      }
      Function *caller = callInst->getFunction();
      Function *callee = callInst.getCalledFunction();
      if (callee && demangleName(callee->getName()) == "pthread_create" && arg_no == 3) {
        if (DBG) errs() << callInst.getArgOperand(2)->getName() << '\n';
        callee = getFunctionWithName(demangleName(callInst.getArgOperand(2)->getName()), M);
        arg_no -= 3;
      }

      if (isIncompatibleFun(callee) || callee->isVarArg()) continue;
      callGraph->addEdge(caller, callee);
      Argument *arg = callee->arg_begin() + arg_no;
      insertElement(arg, walk);

      for (inst_iterator I = inst_begin(callee), E = inst_end(callee); I != E;
           ++I) {
        if (ReturnInst *returnInst = dyn_cast<ReturnInst>(&*I)) {
          returnCallMap[returnInst].push_back(ii);
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
  if (DBG) errs() << "begin insert: " << *elem << '\n';
  // if (isa<Constant>(elem) && !isa<GlobalValue>(elem)) return;
  if (visited.insert(elem).second) {
    // Ignore constants including global variable, so that we will not
    // reference `ret 0` from one function to another.
    // TODO: what if there is data from from global variable to a variable
    // before reaching the checker?
    if (isa<Constant>(elem)) {
      // Special case: global variable is used as an operand
      // TODO: add field allocation chain for usage check
      if (ConstantExpr *expr = dyn_cast<ConstantExpr>(elem)) {
        if (expr->getOpcode() == Instruction::GetElementPtr &&
            isa<GlobalVariable>(expr->getOperand(0)))
          visited.insert(expr->getOperand(0));
      }
      return;
    }
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
