#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
struct ArgumentCounter : public FunctionPass {
  static char ID;
  ArgumentCounter() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    for (auto it = F.arg_begin(); it != F.arg_end(); it++) {
      errs() << it->getArgNo() << "\n";
    }
    return false;
  }
};  // end of struct ArgumentCounter
}  // end of anonymous namespace

char ArgumentCounter::ID = 1;
static RegisterPass<ArgumentCounter> X("hello", "ArgumentCounter World Pass",
                                       false /* Only looks at CFG */, false);
