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

// const bool DBG = true;
const bool DBG = false;

#define unlikely(x) __builtin_expect(!!(x), 0)

size_t FieldChain::calc_hash(const FieldChainElem *chain) {
  if (chain == nullptr) return 0;
  size_t hash = chain->next.hash();
  switch (chain->type) {
  case FieldChainElem::type::field:
    hash = std::hash<ssize_t>{}(chain->field.field_no) ^ (hash << 1);
    break;
  case FieldChainElem::type::deref:
    hash = std::hash<int32_t>{}(0xdeadbeef) ^ (hash << 1);
    break;
  case FieldChainElem::type::offset:
    hash = std::hash<ssize_t>{}(chain->offset.offset) ^ (hash << 1);
    break;
  case FieldChainElem::type::call:
    hash = std::hash<Function *>{}(chain->call.fun) ^ (hash << 1);
    hash = std::hash<size_t>{}(chain->call.arg_no) ^ (hash << 1);
    break;
  }
  return hash;
}

raw_ostream &llvm::defuse::operator<<(raw_ostream &os, const FieldChain &chain) {
  FieldChainElem *node = &*chain;
  char s[20];
  sprintf(s, "#%016lx", chain.hash());
  os << s << '{';
  if (node)
    os << "outermost";
  while (node) {
    switch (node->type) {
    case FieldChainElem::type::offset:
      os << "@(" << node->offset.offset << ')'; break;
    case FieldChainElem::type::field:
      os << "." << "field(" << node->field.field_no << ")" /*node->field.type */; break;
    case FieldChainElem::type::call:
      break;
    case FieldChainElem::type::deref:
      os << '^'; break;
    default:
      os << "(unknown)"; break;
    }
    node = &*node->next;
  }
  os << '}';
  return os;
}

// TODO: Refine def-use chain explanation and eliminate CallGraph
static void explainDefUseChain(const UserGraph::UserNodeList &userList,
                               Instruction *inst, ssize_t last)
{
  errs() << " === Begin explain: ===\n";
  errs() << "Usage point is " << *inst << '\n'
    << "\tin func " << demangleName(inst->getFunction()->getName()) << '\n';
  while (last != -1) {
    auto [elem, chain, prev] = userList[last];
    errs() << "visited " << *elem << " chain is " << chain << " prev is " << prev << '\n';
    if (Instruction *inst = dyn_cast<Instruction>(elem)) {
      errs() << "\t in func " << demangleName(inst->getFunction()->getName()) << '\n';
    } else if (Constant *c = dyn_cast<Constant>(elem)) {
      errs() << "\t is const " << *c << '\n';
    } else if (Argument *arg = dyn_cast<Argument>(elem)) {
      errs() << "\t arg " << *arg
        << " in func " << demangleName(arg->getParent()->getName()) << '\n';
    } else {
      errs() << "\t unknown type " << *elem << '\n';
    }
    last = prev;
  }
  errs() << " === End explain ===\n";
}

static bool functionGlobalVarVisitedHelper(
    Function *fun,
    std::unordered_set<Function*> &visited_fuctions,
    const UserGraph::VisitedNodeSet &visited_values,
    const UserGraph::HitSet &hitPoints,
    const UserGraph::UserNodeList &userList)
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

  for (auto &I : instructions(fun)) {
    Instruction *inst = &I;

    if (hitPoints.find(inst) != hitPoints.end()) {
      errs() << "Is a hit point\n";
      if (DBG)
        explainDefUseChain(userList, inst, hitPoints.find(inst)->second);
      return true;
    }

    if (LoadInst *load = dyn_cast<LoadInst>(inst)) {
      if (DBG) errs() << "    " << *inst << '\n';
      // Is a global variable, and is a pointer type
      Value *op0 = load->getOperand(0);
      // const Type *op0ty = op0->getType();
      if (isa<GlobalVariable>(op0)) {
        if (// op0ty->isPointerTy() && op0ty->getContainedType(0)->isPointerTy() &&
          visited_values.find(op0) != visited_values.end())
        {
          // if (1 || DBG) errs() << "    global var name: " << op0->getName() << '\n';
          // return true;
        }
      } else if (const ConstantExpr *expr = dyn_cast<ConstantExpr>(op0)) {
        // Getting a field of global variable
        if (expr->getOpcode() != Instruction::GetElementPtr) continue;
        Constant *op0op0 = expr->getOperand(0);
        if (isa<GlobalVariable>(op0op0) &&
          visited_values.find(op0op0) != visited_values.end()) {
          // if (1 || DBG) errs() << "    global var name: " << op0op0->getName() << '\n';
          // return true;
        }
      }
    } else if (GetElementPtrInst *getelem = dyn_cast<GetElementPtrInst>(inst)) {
      // Is a global variable, and is a pointer type
      Value *op0 = getelem->getOperand(0);
      // const Type *getelemty = getelem->getType();
      if (isa<GlobalVariable>(op0) &&
          // getelemty->isPointerTy() &&
          visited_values.find(op0) != visited_values.end())
      {
        // if (1 || DBG) errs() << "    global var name: " << op0->getName() << '\n';
        // return true;
      }
    } else if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
      // For newer LLVm, cast to Callbase
      Function *callee = CallSite(inst).getCalledFunction();
      if (functionGlobalVarVisitedHelper(callee, visited_fuctions,
            visited_values, hitPoints, userList))
        return true;
    }
  }
  if (DBG) errs() << " === end global var check === : " << fun->getName() << '\n';
  return false;
}

bool UserGraph::functionGlobalVarVisited(Function *fun) {
  if (DBG) errs() << "End function is : " << fun->getName() << '\n';
  std::unordered_set<Function*> visited_fuctions;
  return functionGlobalVarVisitedHelper(fun, visited_fuctions, visited,
      hitPoints, userList);
}

bool UserGraph::isFunctionVisited(Function *fun) {
  return functionsVisited.count(fun) != 0;
}

void UserGraph::doDFS() {
  visited[root].insert(nullptr);
  visit_stack.push(root);
  while (!visit_stack.empty()) {
    if (isFunctionVisited(target)) return;
    Value *elem = visit_stack.top();
    visit_stack.pop();
    processUser(elem, nullptr, -1, UserGraphWalkType::DFS);
  }
}

void UserGraph::doBFS() {
  if (DBG) errs() << "Begin BFS (root: " << *root << ")\n\n";
  visited[root].insert(nullptr);
  visit_queue.push({root, nullptr, -1});
  /* if (Instruction *ins = dyn_cast<Instruction>(root))
    errs() << " === root is in : " << ins->getFunction()->getName() << '\n'; */
  visit_queue.push({nullptr, nullptr, -1});  // a dummy element marking end of a level
  int level = 0;
  while (!visit_queue.empty()) {
    // if (isFunctionVisited(target)) break;
    auto [head, chain, last] = visit_queue.front();
    if (head != nullptr) {
      processUser(head, chain, last, UserGraphWalkType::BFS);
    }
    visit_queue.pop();  // remove the front element
    if (!visit_queue.empty()) {
      auto [head, chain, last] = visit_queue.front();
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
        visit_queue.push({nullptr, nullptr, -1});
      }
    }
  }
  if (DBG) errs() << "\nEnd BFS\n";
}

/*
 * Decompose GEP into multiple elements in the chain.
 *
 * Any GEP instruction like `GEP $x, n, f1, f2, ...` can be composed by:
 *   $1 = GEP $x, n (if n != 0)
 *   $2 = GEP $1, 0, f1
 *   $3 = GEP $2, 0, f2
 * We store the first relationship as an `offset` which would be common for
 * arrays, and the second and third as `field`s. The elements will be added
 * to the chain in reverse order.
 */
Optional<FieldChain> nest_gep(FieldChain chain, GetElementPtrInst *gep) {
  unsigned operands = gep->getNumOperands();

  // Decompose all fields
  while (operands-- > 2) {
    if (ConstantInt *field = dyn_cast<ConstantInt>(gep->getOperand(operands))) {
      chain = chain.nest_field(NULL, field->getSExtValue());
    } else {
      chain = chain.nest_field(NULL, FieldChainElem::ARRAY_FIELD);
    }
  }

  // Add offset operand to the chain.
  // If it is 0, then skip it. For other constant, add the constant value.
  // If it is a variable, add ARRAY_FIELD to the offset.
  // When doing matching, a variable operand in the given GEP can may either
  // field or const, or a missing 0. Otherwise, it will exact match consts.
  if (ConstantInt *offset = dyn_cast<ConstantInt>(gep->getOperand(1))) {
    // Ignore zero offset. We may consider allow any const offset as
    // array offset in the future (a more relaxed constraint).
    if (!offset->isZero()) {
      chain = chain.nest_offset(NULL, offset->getSExtValue());
    }
  } else {
    // This may be a variable, allow arbitrary array offset
    chain = chain.nest_offset(NULL, FieldChainElem::ARRAY_FIELD);
  }
  return chain;
}

/*
 * Try to match the chain with a GEP instruction. For decomposition rule,
 * refer to `nest_gep` function. The matching will be in GEP operand order.
 *
 * If the argument chain is not root, then it will match the prefix and return
 * the remnant. If the remnant is exactly the root (i.e. nullptr), then the
 * root will be returned. However, if it matched pass the root (i.e. accesses
 * some field), then None will be returned, so the caller will not proceed,
 * but this instruction should be added to the hit points.
 *
 * If the argument chain is already root: then if it only has offset operand,
 * the exact match root will be returned, and hit is set to true. Otherwise,
 * for any field match, it will return None, so the caller will not proceeed.
 *
 * Caller should always check the hit variable.
 *
 * TODO: type matching
 */
Optional<FieldChain> match_gep(FieldChain chain, GetElementPtrInst *gep, bool *hit) {
  const unsigned operands = gep->getNumOperands();
  bool dummy_hit;
  if (!hit) hit = &dummy_hit;
  *hit = false;
  if (unlikely(operands <= 1)) {
    errs() << "Invalid GEP instruction: " << *gep << '\n';
    return None;
  }
  if (chain.get() == nullptr) {
    // Only offset
    if (operands == 2) {
      *hit = true;
      return chain;
    } else {
      return None;
    }
  }

  // Match offset operand first.
  if (ConstantInt *offset = dyn_cast<ConstantInt>(gep->getOperand(1))) {
    // Ignore zero offset.
    if (!offset->isZero()) {
      if (chain->type == FieldChainElem::type::offset &&
          chain->offset.offset == offset->getSExtValue()) {
        chain = chain->next;
      } else {
        return None;
      }
    }
  } else {
    // It may be a variable. We allow either const or var, as long as it is
    // offset type on the chain.
    if (chain->type != FieldChainElem::type::offset)
      return None;
    // This may be a variable, allow arbitrary array offset
    chain = chain->next;
  }
  // Early return if we reached root at any level of the match
  if (chain.get() == nullptr) {
    *hit = true;
    return operands == 2 ? Optional(chain) : None;
  }

  // Match all the fields
  for (unsigned op = 2; op < operands; ++op) {
    if (chain->type != FieldChainElem::type::field)
      return None;
    if (ConstantInt *field = dyn_cast<ConstantInt>(gep->getOperand(op))) {
      if (chain->field.field_no == field->getSExtValue()) {
        chain = chain->next;
      } else {
        return None;
      }
    } else {
      chain = chain->next;
    }
    // Early return if we reached root at any level of the match
    if (chain.get() == nullptr) {
      *hit = true;
      return op == operands - 1 ? Optional(chain) : None;
    }
  }
  return chain;
}

Optional<FieldChain> match_deref(FieldChain chain) {
  if (chain.get() == nullptr || chain->type != FieldChainElem::type::deref)
    return None;
  return chain->next;
}

/*
 * Search for dst if it is src, and search for dst if it is src.
 */
void UserGraph::processUser(Value *elem, const FieldChain &chain, ssize_t last, UserGraphWalkType walk) {
  // bool DBG = true;
  if (DBG) errs() << " === begin process user === : " << *elem << '\n'
                  << "               chain is === : " << chain << '\n';
  if (Instruction *ins = dyn_cast<Instruction>(elem)) {
    Function *fun = ins->getFunction();
    if (DBG) errs() << " func : " << fun->getName() << '\n';
    Type *type = ins->getType();
    functionsVisited[fun].insert(type);
  }

  // We would not expect store instruction could have a user
  if (isa<StoreInst>(elem)) {
    // errs() << "Unexpected store instruction! : " << *elem << '\n';
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
    auto newchain = nest_gep(chain, gep);
    if (newchain == None) return;
    insertElement(gep->getOperand(0), newchain.getValue(), last, walk);
  } else if (LoadInst *load = dyn_cast<LoadInst>(elem)) {
    insertElement(load->getOperand(0), chain.nest_deref(), last, walk);
  }
  // We can also trace the src of pointer cast
  else if (BitCastInst *cast = dyn_cast<BitCastInst>(elem)) {
    insertElement(cast->getOperand(0), chain, last, walk);
  }
  // Constant variable that may contain a global variable or function pointer
  else if (Constant *val = dyn_cast<Constant>(elem)) {
    // Ignore constants except for global variable, so that we will not
    // reference `ret 0` from one function to another.
    // Users of global variable will be added to 

    // We are not interested in bitcast
    val = stripBitCastsAndAlias(val);

    // Special case: global variable is used as an operand
    // TODO: add field allocation chain for usage check
    if (ConstantExpr *expr = dyn_cast<ConstantExpr>(val)) {
      if (expr->getOpcode() == Instruction::GetElementPtr &&
          isa<GlobalVariable>(expr->getOperand(0)))
      {
        auto *gep = dyn_cast<GetElementPtrInst>(expr->getAsInstruction());
        auto newchain = nest_gep(chain, gep);
        delete gep;
        if (newchain.hasValue())
          insertElement(expr->getOperand(0), newchain.getValue(), last, walk);
      } else {
        // Unsupported global var usage
        return;
      }
    } else if (Function *fun = dyn_cast<Function>(val)) {
      (void)fun;
      // TODO: add function pointer into data flow analysis
      return;
      /* auto newchain = match_call(chain, fun);
      if (newchain.hasValue()) {
        if (newchain.getValue().get() == nullptr)
          addHitPoint(load);
      } */
    } else if (isa<GlobalVariable>(val)) {
      // Do nothing, fall to search for users
    } else {
      // Other (ConstantData, etc.), ignore and don't find users
      return;
    }
  }
  // Modification reaches to an argument
  else if (Argument *arg = dyn_cast<Argument>(elem)) {
    processArgument(arg, chain, last, walk);
  }
  // Track data flow of returned value in the caller
  else if (ReturnInst *ret = dyn_cast<ReturnInst>(elem)) {
    Function *this_func = ret->getFunction();
    if (calleeCallerMap.count(this_func) != 0) {
      for (auto inst : calleeCallerMap[this_func])
        insertElement(inst, chain, last, walk);
    } else {
      for (auto user : this_func->users()) {
        if (Instruction *inst = dyn_cast<Instruction>(user)) {
          if (!(isa<CallInst>(inst) || isa<InvokeInst>(inst)))
            continue;
          CallSite call_site(inst);
          auto [caller, callee, arg_no] = extractCallerCallee(call_site, -1);
          if (isIncompatibleFun(caller) || this_func != callee) continue;
          callGraph->addEdge(this_func, caller);
          calleeCallerMap[this_func].insert(inst);
          // TODO: handle pointer passing: add pointer to data flow analysis
          insertElement(user, chain, last, walk);
        }
      }
    }
  }
  // Modification was in a return value, then add the return instructions
  // in that call
  // TODO: cleanup duplicate code
  // TODO: Add option to enable this, otherwise MySQL analysis is too slow,
  // and generates much more false positives
  /* else if (isa<CallInst>(elem) || isa<InvokeInst>(elem)) {
    CallSite call(elem);
    auto [caller, callee, arg_no] = extractCallerCallee(call, 0);
    if (callee && !isIncompatibleFun(callee)) {
      callGraph->addEdge(caller, callee);
      for (auto &I : instructions(callee)) {
        if (ReturnInst *ret = dyn_cast<ReturnInst>(&I)) {
          insertElement(ret->getOperand(0), chain, last, walk);
        }
      }
    }
  } */

  /*
   * One limitation: we currently do not support pointer arithmetic, but only
   * accept LLVM-builtin field operations. This could be a future work, and
   * the instructions that needs to be supported are PtrToIntInst,
   * IntToPtrInst. We can add information about pointer state (integer or
   * pointer) to FieldChainElem, and follow BinaryOperator instructions.
   * But we should exclude cases like pointer subtractions to get index.
   */

  /*
   * As src, find dst (find normal out-degree data flow)
   *
   * In general, add LHS to the search, only if it matches the field chain.
   * For those users that deref to the root, mark its function as usage point.
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
        insertElement(store->getOperand(1), chain.nest_deref(), last, walk);
      } else {
        // If it was the dst operand, search for usage of src, remove one
        // deref from chain.
        auto newchain = match_deref(chain);
        if (newchain.hasValue()) {
          if (newchain.getValue().get() == nullptr)
            addHitPoint(store, last);
          insertElement(store->getOperand(0), newchain.getValue(), last, walk);
        }
      }
    } else if (LoadInst *load = dyn_cast<LoadInst>(user)) {
      auto newchain = match_deref(chain);
      if (newchain.hasValue()) {
        if (newchain.getValue().get() == nullptr)
          addHitPoint(load, last);
        insertElement(user, newchain.getValue(), last, walk);
      }
    } else if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(user)) {
      bool hit;
      auto newchain = match_gep(chain, gep, &hit);
      if (hit)
        addHitPoint(gep, last);
      if (newchain.hasValue())
        insertElement(user, newchain.getValue(), last, walk);
    } else if (isa<ExtractValueInst>(user)) {
      // TODO
      errs() << "Unsupported Instruction: " << *user << '\n';
    } else if (isa<InsertValueInst>(user)) {
      // TODO
      errs() << "Unsupported Instruction: " << *user << '\n';
    } else if (isa<CallInst>(user) || isa<InvokeInst>(user)) {
      processCall(CallSite(user), elem, chain, last, walk);
    }
    // Constant that may contain global variable
    else if (Constant *c = dyn_cast<Constant>(user)) {
      c = stripBitCastsAndAlias(c);

      // errs() << "stripped const " << *c << '\n';
      if (ConstantExpr *expr = dyn_cast<ConstantExpr>(c)) {
        // errs() << "stripped constexpr " << *expr << '\n';
        if (expr->getOpcode() == Instruction::GetElementPtr &&
            isa<GlobalVariable>(expr->getOperand(0)) &&
            expr->getOperand(0) == elem)
        {
          auto *gep = dyn_cast<GetElementPtrInst>(expr->getAsInstruction());
          // gep->getType()->get
          auto newchain = match_gep(chain, gep, nullptr);
          delete gep;
          if (newchain.hasValue())
            insertElement(c, newchain.getValue(), last, walk);
        } else {
          // Unsupported global var usage
          continue;
        }
      } else if (Function *fun = dyn_cast<Function>(c)) {
        (void)fun;
        // TODO: add function pointer into data flow analysis
        continue;
        /* auto newchain = match_call(chain, fun);
        if (newchain.hasValue()) {
          if (newchain.getValue().get() == nullptr)
            addHitPoint(load);
        } */
      } else if (isa<GlobalVariable>(c)) {
        // Do nothing, fall to search for users
        insertElement(user, chain, last, walk);
      } else {
        // Other (ConstantData, etc.), ignore and don't find users
        continue;
      }
    }
    // Those that are as-is: pointer cast, return, ternary operators
    else if (isa<BitCastInst>(user) || isa<ReturnInst>(user) ||
             isa<PHINode>(user) || isa<SelectInst>(user)) {
      insertElement(user, chain, last, walk);
    } else {
      // Other thing: ignore or output error
      if (isa<Instruction>(user)) {
        // Things to ignore:
        // - other binary operations and other cast operations (currently
        //   we don't support pointer arithmetics)
        // - cmp operations & control flow operations (no data flow)
        if (isa<BinaryOperator>(user) || isa<CastInst>(user) ||
                 isa<CmpInst>(user) || isa<TerminatorInst>(user)) {
          // Just ignore it
        } else {
          // errs() << "Unsupported Instruction: " << *user << '\n';
        }
      } else if (isa<Constant>(user)) {
        errs() << "TODO: constexpr: " << *user << "\n";
      } else {
        errs() << "Unsupported user: " << *user << '\n';
      }
    }
  }
}

void UserGraph::processCall(CallSite call, Value *arg, const FieldChain &chain,
                            ssize_t last, UserGraphWalkType walk)
{
  Instruction *inst = call.getInstruction();
  unsigned int arg_no;
  for (arg_no = 0; arg_no < call.getNumArgOperands(); arg_no++) {
    if (call.getArgOperand(arg_no) == arg) break;
  }
  auto [caller, callee, new_arg_no] = extractCallerCallee(call, arg_no);
  arg_no = new_arg_no;
  // Still cannot get callee with either direct or indirect call, skip.
  if (!callee) return;

  if (isIncompatibleFun(callee) || callee->isVarArg()) return;
  callGraph->addEdge(caller, callee);
  Argument *passed_arg = callee->arg_begin() + arg_no;
  insertElement(passed_arg, chain, last, walk);

  // TODO: Take care of arg shift like pthread_create
  calleeCallerMap[callee].insert(inst);
}

void UserGraph::processArgument(Argument *arg, const FieldChain &chain,
    ssize_t last, UserGraphWalkType walk)
{
  // Must be a pointer argument
  if (!arg->getType()->isPointerTy()) return;
  // Must not be the arg itself
  // if (chain.get() == nullptr) return;
  if (DBG) errs() << "Is an argument\n";
  Function *fun = arg->getParent();
  if (DBG) errs() << "    function name is: " << fun->getName() << "\n";

  auto add_caller_arg = [this, arg, &chain, last, walk, fun](CallSite call_site) {
    // TODO: handle indirect call
    if (call_site.getCalledFunction() != fun)
      return;
    Value *caller_arg = call_site.getArgOperand(arg->getArgNo());
    if (DBG) errs() << "    func user: " << *call_site.getInstruction() << '\n';
    if (DBG) errs() << "    func arg: " << arg->getArgNo()
      << " user: " << *caller_arg << '\n';
    insertElement(caller_arg, chain, last, walk);
  };

  // Has caller
  if (calleeCallerMap.find(fun) != calleeCallerMap.end()) {
    for (Instruction *call_inst : calleeCallerMap.find(fun)->second) {
      add_caller_arg(CallSite(call_inst));
    }
  } else {
    for (User *call_inst : fun->users()) {
      if (isa<CallInst>(call_inst) || isa<InvokeInst>(call_inst)) {
        add_caller_arg(CallSite(call_inst));
      } else if (ConstantExpr *c = dyn_cast<ConstantExpr>(call_inst)) {
        for (User *call : c->users()) {
          if (isa<CallInst>(call) || isa<InvokeInst>(call))
            add_caller_arg(CallSite(call));
        }
      }
      // TODO: function pointer data flow
    }
  }
}

void UserGraph::insertElement(Value *elem, const FieldChain &chain,
                              ssize_t last, UserGraphWalkType walk)
{
  std::unordered_set<FieldChain> &visited_elem = visited[elem];
  bool is_new_chain = visited_elem.insert(chain).second;
  if (is_new_chain && visited_elem.size() <= 3) {
    if (DBG) errs() << "        insert: " << *elem << '\n'
                    << "         chain: " << chain << '\n';
    if (walk == UserGraphWalkType::DFS)
      visit_stack.push(elem);
    else {
      ssize_t next = userList.size();
      userList.push_back({elem, chain, last});
      visit_queue.push({elem, chain, next});
    }
  } else {
    if (DBG) errs() << "        insert failure: is_new_chain=" << is_new_chain
      << " size=" << visited_elem.size() << '\n';
  }
  if (DBG) {
    for (auto chain : visited_elem)
      errs() << "         new chain has: " << chain << '\n';
  }
}

void UserGraph::addHitPoint(Instruction *inst, ssize_t last) {
  hitPoints.insert({inst, last});
}

bool UserGraph::isIncompatibleFun(Function *fun) {
  if (fun == NULL || fun->isIntrinsic()) return true;
  std::string name = demangleName(fun->getName());
  if (name.rfind("std", 0) == 0 || name.rfind("boost", 0) == 0 ||
      name.rfind("ib::logger", 0) == 0 || name.rfind("PolicyMutex", 0) == 0 ||
      name.rfind("__gnu", 0) == 0 || name.rfind("ut_allocator", 0) == 0 ||
      alloc_rules.should_ignore(fun)) {
    return true;
  }
  return false;
}
