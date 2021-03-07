#include "llvm/ADT/MapVector.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

typedef llvm::MapVector<const llvm::Function *, unsigned> ResultStaticCC;

static void printResults(llvm::raw_ostream &OutS,
                         const ResultStaticCC &DirectCalls);

struct StaticCallCounter : public llvm::ModulePass {
  static char ID;
  ResultStaticCC results;
  StaticCallCounter() : llvm::ModulePass(ID) {}

  ResultStaticCC getStaticCalls(llvm::Module &M) {
    ResultStaticCC result;

    for (auto &Func : M) {
      for (auto &BB : Func) {
        for (auto &Ins : BB) {
          // Check if the instruction is a calling instruction
          if (auto CI = dyn_cast<CallInst>(&Ins)) {
            auto FunctionCall = CI->getCalledFunction();
            if (nullptr == FunctionCall) continue;

            auto CallCount = result.find(FunctionCall);
            if (CallCount == result.end()) {
              CallCount = result.insert(std::make_pair(FunctionCall, 0)).first;
            }
            ++CallCount->second;
          }
        }
      }
    }

    return result;
  }

  bool runOnModule(llvm::Module &M) override {
    results = getStaticCalls(M);
    return false;
  }

  void print(raw_ostream &O, const Module *M) const override {
    printResults(O, results);
  }
};

void printResults(llvm::raw_ostream &OutS, const ResultStaticCC &DirectCalls) {
  for (auto &Call : DirectCalls) {
    OutS << Call.first->getName().str().c_str() << " " << Call.second << "\n";
  }
}

char StaticCallCounter::ID = 2;
RegisterPass<StaticCallCounter> X(/*PassArg=*/"legacy-static-cc",
                                  /*Name=*/"LegacyStaticCallCounter",
                                  /*CFGOnly=*/true,
                                  /*is_analysis=*/true);