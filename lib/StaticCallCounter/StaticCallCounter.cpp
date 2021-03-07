#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
struct StaticCallCounter : public FunctionPass {
  static char ID;
  StaticCallCounter() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    for(auto it = F.arg_begin(); it != F.arg_end(); it++) {
      errs() << it->getArgNo() << "\n";
    }
    return false;
  }
}; // end of struct StaticCallCounter
}  // end of anonymous namespace

char StaticCallCounter::ID = 1;
static RegisterPass<StaticCallCounter> X("hello", "StaticCallCounter World Pass",
                             false /* Only looks at CFG */,
