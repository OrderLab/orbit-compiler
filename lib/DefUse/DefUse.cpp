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

/*
 * Search for dst if it is src, and search for dst if it is src.
 */
void UserGraph::processUser(Value *elem, UserGraphWalkType walk) {
  // bool DBG = true;
  if (DBG) errs() << " === begin process user === : " << *elem << '\n';
  if (Instruction *ins = dyn_cast<Instruction>(elem)) {
    Function *fun = ins->getFunction();
    if (DBG) errs() << "    in func: " << fun->getName() << '\n';
    Type *type = ins->getType();
    functionsVisited[fun].insert(type);
  }

  if (DBG) errs() << "    inst: " << *elem << '\n';

  // TODO: Move Constant handling from insert to here

  // We would not expect store instruction could have a user
  if (isa<StoreInst>(elem)) {
    errs() << "Unexpected store instruction! : " << *elem << '\n';
    return;
  }

  /*
   * As dst, find src (find definition, i.e. find data flow to this var)
   *
   * For GEP, find definition of operand that was added by offset. Add offset
   * to the chain. If the offset is variable, consider it as array.
   *
   * For Load, find definition of src operand. Add deref to the chain.
   *
   * One exception is that src finding for StoreInst is merged into the
   * following loop, only if it was the src operand. Then add dst and add
   * deref to the chain.
   */
  if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(elem)) {
    // TODO: add pointer chain info
    insertElement(gep->getOperand(0), walk);
  }
  if (LoadInst *load = dyn_cast<LoadInst>(elem)) {
    // TODO: add pointer chain info
    insertElement(load->getOperand(0), walk);
  }
  // TODO: case for global variable
  // TODO: case for argument


  // Modification to an argument
  if (Argument *arg = dyn_cast<Argument>(elem)) {
    // Must be a pointer argument
    if (!arg->getType()->isPointerTy()) return;
    if (DBG) errs() << "Is an argument\n";
    Function *fun = arg->getParent();
    if (DBG) errs() << "    function is:\n" << *fun << "    function end\n";
    for (Value *call : fun->users()) {
      if (isa<CallInst>(call) || isa<InvokeInst>(call)) {
        if (DBG) errs() << "    function user: " << *call << '\n';
        if (DBG) errs() << "    function arg: " << arg->getArgNo()
          << " user: " << *CallSite(call).getArgOperand(arg->getArgNo()) << '\n';
        Value *user = CallSite(call).getArgOperand(arg->getArgNo());
        insertElement(user, walk);
      }
    }
    // TODO: function pointer data flow
  }

  if (ReturnInst *ret = dyn_cast<ReturnInst>(elem)) {
    if (returnCallMap.count(ret) != 0) {
      for (auto inst : returnCallMap[ret]) {
        insertElement(inst, walk);
      }
    } else {
      Function *fun = ret->getFunction();
      for (auto user : fun->users()) {
        if (Instruction *ins = dyn_cast<Instruction>(user)) {
          Function *caller = ins->getFunction();
          if (isIncompatibleFun(caller)) continue;
          callGraph->addEdge(fun, caller);
        }
        // TODO: handle pointer passing: add pointer to data flow analysis
        insertElement(user, walk);
      }
    }
  }

  // TODO: PtrToIntInst, IntToPtrInst


  /*
   * As src, find dst (find out-degree data flow)
   *
   * In general, add LHS to the search, only if it matches the field chain.
   * For those user that deref to the root, mark its function as usage point.
   * ^TODO
   *
   * One exception is that src finding for StoreInst is merged into this loop.
   */
  for (User *user : elem->users()) {
    if (DBG) errs() << "    user: " << *user << '\n';
    if (StoreInst *store = dyn_cast<StoreInst>(user)) {
      if (elem == store->getOperand(0)) {
        // If it was the src operand, search for definition of dst, add deref
        // to the chain.
        // TODO
        insertElement(store->getOperand(1), walk);
      } else {
        // If it was the dst operand, search for usage of src, remove one
        // deref from chain.
        // TODO
        insertElement(store->getOperand(0), walk);
      }
    } else if (isa<LoadInst>(user)) {
      // TODO
      insertElement(user, walk);
    } else if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(user)) {
      // TODO
      insertElement(user, walk);
    } else if (ExtractValueInst *val = dyn_cast<ExtractValueInst>(user)) {
      // TODO
      insertElement(user, walk);
    } else if (InsertValueInst *val = dyn_cast<InsertValueInst>(user)) {
      // TODO
      insertElement(user, walk);
    } else if (isa<CallInst>(user) || isa<InvokeInst>(user)) {
      // TODO
      processCall(CallSite(user), elem, walk);
    } else if (isa<BitCastInst>(user)) {
      insertElement(user, walk);
    } else if (isa<ReturnInst>(user)) {
      insertElement(user, walk);
    } else {
      if (isa<Instruction>(user)) {
        // Things to pass as-is: ternary operators
        if (isa<PHINode>(user) || isa<SelectInst>(user)) {
          insertElement(user, walk);
        }
        // Things to ignore:
        // - other binary operations and other cast operations (currently
        //   we don't support pointer arithmetics)
        // - cmp operations & control flow operations (no data flow)
        else if (isa<BinaryOperator>(user) || isa<CastInst>(user) ||
                 isa<CmpInst>(user) || isa<TerminatorInst>(user)) {
          // Just ignore it
        } else {
          errs() << "Unsupported Instruction: " << *user << '\n';
        }
      } else if (isa<Constant>(user)) {
        errs() << "TODO: constexpr: " << *user << "\n";
      } else {
        errs() << "Unsupported user: " << *user << '\n';
      }
    }
  }
}

void UserGraph::processCall(CallSite call, Value *arg, UserGraphWalkType walk) {
  Instruction *inst = call.getInstruction();
  unsigned int arg_no;
  for (arg_no = 0; arg_no < call.getNumArgOperands(); arg_no++) {
    if (call.getArgOperand(arg_no) == arg) break;
  }
  Function *caller = call->getFunction();
  Function *callee = call.getCalledFunction();
  if (callee && demangleName(callee->getName()) == "pthread_create" && arg_no == 3) {
    if (DBG) errs() << call.getArgOperand(2)->getName() << '\n';
    callee = getFunctionWithName(demangleName(call.getArgOperand(2)->getName()), M);
    arg_no -= 3;
  }

  if (isIncompatibleFun(callee) || callee->isVarArg()) return;
  callGraph->addEdge(caller, callee);
  Argument *passed_arg = callee->arg_begin() + arg_no;
  insertElement(passed_arg, walk);

  for (inst_iterator I = inst_begin(callee), E = inst_end(callee); I != E;
      ++I) {
    if (ReturnInst *returnInst = dyn_cast<ReturnInst>(&*I)) {
      returnCallMap[returnInst].push_back(inst);
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
