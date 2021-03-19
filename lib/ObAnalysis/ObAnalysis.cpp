#include <set>
#include "llvm/ADT/MapVector.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

struct ObAnalysis : public llvm::ModulePass {
  static char ID;
  std::set<std::string> variableList;

  ObAnalysis() : llvm::ModulePass(ID) {}

  void exploreInstructions(Function &function) {
    // Return for function declarations
    if (function.isDeclaration()) {
      return;
    }

    // TODO: Prevent same function being called multiple times

    errs() << "Exploring Function: " << function.getName().str() << "\n";
    for (auto &basicBlock : function) {
      for (auto &ins : basicBlock) {
        // Check if instruction is call instruction
        if (auto callIns = dyn_cast<CallInst>(&ins)) {
          auto functionCall = callIns->getCalledFunction();
          if (nullptr != functionCall) {
            // DFS into new function call
            exploreInstructions(*functionCall);
          }
        } else {
          for (auto &bb : function) {
            for (auto &ins : bb) {
              for (unsigned int i = 0; i < ins.getNumOperands(); i++) {
                auto *value = ins.getOperand(i);
                if (value->hasName()) {
                  variableList.insert(value->getName().str());
                  // errs() << "Variable: " << value->getName() << "\n";
                }
              }
            }
          }
        }
      }
    }
    return;
  }

  void print(raw_ostream &O, const Module *M) const override {
    for (auto variable : variableList) {
      errs() << variable << " \n";
    }
  }

  bool runOnModule(llvm::Module &M) override {
    for (auto &Func : M) {
      if (Func.getName().str() == "calls") {
        exploreInstructions(Func);
      }
    }
    return false;
  }
};

char ObAnalysis::ID = 3;
RegisterPass<ObAnalysis> X(/*PassArg=*/"legacy-static-cc",
                           /*Name=*/"ObAnalysis",
                           /*CFGOnly=*/true,
                           /*is_analysis=*/true);