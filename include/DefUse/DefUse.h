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
#include <unordered_set>

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

struct FieldChainElem;
class FieldChain : public std::shared_ptr<FieldChainElem> {
  size_t _hash;
  static size_t calc_hash(const FieldChainElem *chain);
public:
  FieldChain(FieldChainElem *chain)
    : std::shared_ptr<FieldChainElem>(chain), _hash(calc_hash(chain)) {}
  FieldChain nest_offset(Type *type, ssize_t offset);
  FieldChain nest_field(Type *type, ssize_t offset);
  FieldChain nest_deref(void);
  FieldChain nest_call(Function *fun, size_t arg_no);
  bool operator==(const FieldChain &rhs) const;
  size_t hash() const { return _hash; }
};
raw_ostream &operator<<(raw_ostream &os, const FieldChain &chain);

struct FieldChainElem {
  // special offset to indicate array element
  static const ssize_t ARRAY_FIELD = LONG_MAX;

  enum class type { offset, field, deref, call } type;
  union {
    struct { Type *type; ssize_t offset; } offset;
    struct { Type *type; ssize_t field_no; } field;
    struct { Function *fun; size_t arg_no; } call;
  };
  // maybe: optimize alloc/dealloc of shared_ptr to use lifecycle-based
  FieldChain next;
};

inline FieldChain FieldChain::nest_offset(Type *type, ssize_t offset) {
  return FieldChain(new FieldChainElem{
    FieldChainElem::type::offset, { .offset = {type, offset} }, {*this}
  });
}
inline FieldChain FieldChain::nest_field(Type *type, ssize_t field_no) {
  return FieldChain(new FieldChainElem{
    FieldChainElem::type::field, { .field = {type, field_no} }, {*this}
  });
}
inline FieldChain FieldChain::nest_deref() {
  return FieldChain(new FieldChainElem{ FieldChainElem::type::deref, {}, {*this} });
}
inline FieldChain FieldChain::nest_call(Function *fun, size_t arg_no) {
  return FieldChain(new FieldChainElem{
    FieldChainElem::type::call, { .call = {fun, arg_no} }, {*this}
  });
}
inline bool FieldChain::operator==(const FieldChain &rhs) const {
  if (this->hash() != rhs.hash()) return false;
  FieldChainElem *l = &**this, *r = &*rhs;
  while (l && r) {
    if (l->type != r->type) return false;
    switch (l->type) {
    case FieldChainElem::type::field:
      if (l->field.field_no != r->field.field_no) return false;
      break;
    case FieldChainElem::type::offset:
      if (l->offset.offset != r->offset.offset) return false;
      break;
    case FieldChainElem::type::call:
      if (l->call.fun != r->call.fun || l->call.arg_no != r->call.arg_no)
        return false;
      break;
    case FieldChainElem::type::deref: break;
    }
    l = &*l->next;
    r = &*r->next;
  }
  return !l && !r;
}

} // namespace defuse
} // namespace llvm

template<>
struct std::hash<llvm::defuse::FieldChain> {
  std::size_t operator()(llvm::defuse::FieldChain const& s) const noexcept {
    return s.hash();
  }
};


// TODO: We should restructure the files and put utilities in another file
// TODO: lifecycle based alloc/dealloc rule
// TODO: turn this into CLI arguments
struct AllocRules {
  // Current rule for alloc does not allow data flow into allocation
  // functions, and the allocated pointer defaults to the return value.
  std::set<std::string> alloc;
  // Dealloc and realloc allow specifying the argument number of
  // previously allocated value.
  std::map<std::string, unsigned> dealloc;
  std::map<std::string, unsigned> realloc;
  // If the provided string has a single '*' character, it will ignore
  // all functions that is prefixed with the name before '*'. Otherwise,
  // use exact matching. This will be the last rule to match on.
  std::set<std::string> ignored;

  bool should_ignore(const std::string &name) const;
};


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
  // Current value or use point, then the idx of last one (-1 means the start)
  typedef std::tuple<Value *, FieldChain, ssize_t> UserNode;
  typedef std::vector<UserNode> UserNodeList;
  typedef std::unordered_map<Value *, std::unordered_set<FieldChain>> VisitedNodeSet;
  typedef std::queue<std::tuple<Value *, FieldChain, ssize_t>> VisitQueue;
  typedef std::stack<Value *> VisitStack;
  typedef std::unordered_map<Function *, std::set<Type *>> FunctionSet;
  typedef std::unordered_map<Instruction *, std::vector<Instruction *>>
      ReturnCallMap;
  typedef std::unordered_map<Instruction *, ssize_t> HitSet;

  UserGraph(Value *v, CallGraph *cg, Function *target,
            const AllocRules &alloc_rules, int depth = -1)
    : functionsVisited(),
      root(v), maxDepth(depth), callGraph(cg), target(target),
      alloc_rules(alloc_rules)
  {}
  ~UserGraph() {}

  void run(UserGraphWalkType t = UserGraphWalkType::DFS) {
    if (t == UserGraphWalkType::DFS)
      doDFS();
    else
      doBFS();
  }

  FunctionSet::iterator func_begin() { return functionsVisited.begin(); }
  FunctionSet::iterator func_end() { return functionsVisited.end(); }

  bool isFunctionVisited(Function *);
  bool functionGlobalVarVisited(Function *fun);
  void printCallSite();

 protected:
  void doDFS();
  void doBFS();

 private:
  bool isIncompatibleFun(Function *fun);
  void processUser(Value *elem, FieldChain chain, ssize_t last, UserGraphWalkType walk);
  void processCall(CallSite call, Value *arg, FieldChain chain, ssize_t last, UserGraphWalkType walk);
  void insertElement(Value *elem, FieldChain chain, ssize_t last, UserGraphWalkType walk);
  void addHitPoint(Instruction *inst, ssize_t last);

 public:
  FunctionSet functionsVisited;
  std::vector<Function *> funVector;

 private:
  Value *root;
  int maxDepth;
  UserNodeList userList;
  VisitedNodeSet visited;
  VisitQueue visit_queue;
  VisitStack visit_stack;
  CallGraph *callGraph;
  Function *target;
  ReturnCallMap returnCallMap;
  HitSet hitPoints;
  const AllocRules &alloc_rules;
};

}  // namespace defuse
}  // namespace llvm

#endif /* __DEFUSE_H_ */
