// The Obi-wan Project
//
// Copyright (c) 2020, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include <iostream>

#include <llvm/Support/CommandLine.h>

#include "Utils/LLVM.h"
#include "Utils/String.h"

using namespace std;
using namespace llvm;

cl::list<std::string> TargetFunctions("function", cl::desc("<Function>"),
                                      cl::ZeroOrMore);
cl::opt<string> inputFilename(cl::Positional, cl::desc("<input file>"),
                              cl::Required);

void extractFunc(Function *F) {}

int main(int argc, char *argv[]) {
  cl::ParseCommandLineOptions(argc, argv);

  LLVMContext context;
  unique_ptr<Module> M = parseModule(context, inputFilename);
  if (!M) {
    errs() << "Failed to parse '" << inputFilename << "' file:\n";
    return 1;
  }
  set<string> targetFunctionSet(TargetFunctions.begin(), TargetFunctions.end());
  set<Function *> Functions;
  for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I) {
    Function &F = *I;
    errs() << "Func name is " << F.getName() << "pointer " << &F << "\n";
    if (!F.isDeclaration()) {
      if (targetFunctionSet.empty() ||
          targetFunctionSet.count(F.getName()) != 0) {
        errs() << "Extracting variables from " << F.getName() << "()\n";
        extractFunc(&F);
        Functions.insert(&F);
      }
    }
  }
}
