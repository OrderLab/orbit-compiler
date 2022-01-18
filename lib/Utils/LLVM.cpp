// The Obi-wan Project
//
// Copyright (c) 2020, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//
// LLVM related functions
//
//  Author: Ryan Huang <huang@cs.jhu.edu>
//

#include "Utils/LLVM.h"
#include "llvm/IR/Constants.h"

using namespace std;
using namespace llvm;

unique_ptr<Module> parseModule(LLVMContext &context, string inputFile) {
  SMDiagnostic SMD;

#if ((LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR <= 5))
  auto _M = ParseIRFile(inputFile, SMD, context);
  auto M = unique_ptr<Module>(_M);
#else
  auto M = parseIRFile(inputFile, SMD, context);
#endif

  if (!M) {
    SMD.print("llvm utils", errs());
  }
  return M;
}

// Helper function to demangle a function name given a mangled name
// Note: This strips out the function arguments along with the function number
std::string demangleName(std::string mangledName) {
  if (mangledName.size() == 0) return "";

  const char *mangled = mangledName.c_str();
  char *buffer = (char *)malloc(strlen(mangled));
  size_t length = strlen(mangled);
  int status;
  char *demangled = itaniumDemangle(mangled, buffer, &length, &status);

  if (demangled != NULL) {
    std::string str(demangled);
    // Strip out the function arguments
    size_t pos = str.find_first_of("(");
    free(demangled);
    return str.substr(0, pos);
  }
  free(demangled);
  return mangledName;
}

std::string demangleFunctionName(Function *fun) {
  return demangleName(fun->getName());
}

// Helper function to figure out if a function is a constructor. LLVM does not
// seem to support this so we do it manually by demangling the function name
bool isConstructor(Value *val) {
  if (Instruction *ins = dyn_cast<Instruction>(val)) {
    Function *f = ins->getFunction();
    std::string name = demangleName(f->getName());
    if (name.length() == 0) return false;
    int index = name.find("::");
    if (index == -1) return false;
    std::string str1 = name.substr(index + 2);
    if (str1.compare(name.substr(0, str1.length())) == 0) return true;
  }
  return false;
}

// Helper function to find a function given the name. Internally demangles the
// name
Function *getFunctionWithName(std::string name, Module &M) {
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function &F = *I;
    std::string demangled = demangleName(F.getName());
    // Search for exact match, but truncate parameters including parenthesis.
    size_t pos = demangled.find('(');
    if (pos != std::string::npos)
      demangled.resize(pos);
    if (demangled == name)
      return &F;
  }
  return NULL;
}

// Gets functions belonging to the same class
// We do this by examining the first argument of the function
std::vector<Function *> getFunctionsWithType(Type *type, Module &M) {
  std::vector<Function *> funs;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function &F = *I;
    if (F.arg_size() == 0) continue;
    Argument *arg = F.arg_begin();
    // TODO: not considering inheritance
    if (arg->getType() == type) funs.push_back(&F);
  }
  return funs;
}

// Determines if two instructions access the same member variable
// This should work at an interprocedural level which means we cannot use
// `isIdenticalTo()` since that would only work for identical operands
bool isAccessingSameStructVar(const GetElementPtrInst *inst1,
                              const GetElementPtrInst *inst2) {
  if (!inst1->isSameOperationAs(inst2)) return false;
  if (inst1->getNumIndices() != inst2->getNumIndices()) return false;
  int num_indices = inst1->getNumIndices();
  for (int i = 0; i < num_indices; i++) {
    if (*(inst1->idx_begin() + i) != *(inst2->idx_begin() + i)) return false;
  }
  return true;
}

void printCallSite(Value *val) {
  Instruction *ins = dyn_cast<Instruction>(val);
  MDNode *metadata = ins->getMetadata(0);
  if (metadata == NULL) return;
  DILocation *debugLocation = dyn_cast<DILocation>(metadata);
  if (debugLocation == NULL) return;
  errs() << "Found Heap Allocation in "
         << "\n"
         << "File: " << debugLocation->getFilename() << "\n"
         << "Function: " << demangleFunctionName(ins->getFunction()) << "\n"
         << "Line: " << debugLocation->getLine() << "\n"
         << "Column: " << debugLocation->getColumn() << "\n"
         << "Return Type: " << *(ins->getFunction()->getReturnType()) << "\n";
}

// TODO: change this to resolveCallee
std::tuple<Function *, Function *, unsigned>
extractCallerCallee(CallSite call, unsigned arg_no) {
  Function *caller = call.getCaller();
  Function *callee = call.getCalledFunction();
  Value *called_value = nullptr;

  // If it is an indirect call, try to extract the callee
  if (!callee) {
    called_value = call.getCalledValue();
  } else if (demangleName(callee->getName()) == "pthread_create" && arg_no == 3) {
    // reset callee so we will not search into pthread_create
    callee = nullptr;
    called_value = call.getArgOperand(2);
    arg_no -= 3;
  } else {
    // Do nothing
  }

  // We need to extract the actual callee
  if (called_value) {
    if (ConstantExpr *expr = dyn_cast<ConstantExpr>(called_value)) {
      // First case: simple function pointer cast
      if (Function *fun = dyn_cast<Function>(expr->stripPointerCasts())) {
        callee = fun;
      } else {
        errs() << "Unknown function call to : " << *expr << '\n'
          << " opcode name: " << expr->getOpcodeName() << " op0: " << *expr->getOperand(0) << '\n';
      }
    } else {
      // TODO: Second case: probably a `load`ed value, try match the chain
    }
  }
  return {caller, callee, arg_no};
}

Constant *stripBitCastsAndAlias(Constant *c) {
  if (!c) return nullptr;
  do {
    if (ConstantExpr *expr = dyn_cast<ConstantExpr>(c)) {
      if (expr->getOpcode() == Instruction::BitCast) {
        c = expr->getOperand(0);
      } else {
        break;
      }
    } else if (GlobalAlias *alias = dyn_cast<GlobalAlias>(c)) {
      c = alias->getAliasee();
    } else {
      break;
    }
  } while (true);
  return c;
}
