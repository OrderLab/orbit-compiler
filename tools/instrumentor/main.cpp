// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include <fstream>
#include <iostream>

#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>

#include "Instrument/InstrumentAllocPass.h"
#include "Utils/LLVM.h"
#include "Utils/String.h"

using namespace std;
using namespace llvm;
using namespace llvm::instrument;

cl::opt<string> inputFilename(cl::Positional, cl::desc("<input file>"),
                              cl::Required);
cl::opt<string> outputFilename(
    "o", cl::desc("File to write the instrumented bitcode"),
    cl::value_desc("file"));

cl::opt<bool> usePrintf(
    "use-printf", cl::desc("Whether to instrument using regular printf"));

bool runOnFunction(AllocInstrumenter *instrumenter, Function &F) {
  bool instrumented = false;
  // TODO: replace this demo logic
  Instruction *instr;
  for (inst_iterator ii = inst_begin(F), ie = inst_end(F); ii != ie; ++ii) {
    instr = &*ii;
    instrumented |= instrumenter->instrumentInstr(instr);
  }
  return instrumented;
}

bool saveModule(Module *M, string outFile) {
  if (verifyModule(*M, &errs())) {
    errs() << "Error: module failed verification.\n";
    return false;
  }
  ofstream ofs(outFile);
  raw_os_ostream ostream(ofs);
  WriteBitcodeToFile(M, ostream);
  return true;
}

int main(int argc, char *argv[]) {
  cl::ParseCommandLineOptions(argc, argv);

  LLVMContext context;
  unique_ptr<Module> M = parseModule(context, inputFilename);
  if (!M) {
    errs() << "Failed to parse '" << inputFilename << "' file:\n";
    return 1;
  }
  AllocInstrumenter instrumenter(usePrintf);
  if (!instrumenter.initHookFuncs(M.get(), context)) {
    errs() << "Failed to initialize hook functions\n";
    return 1;
  }
  set<Function *> Functions;
  for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I) {
    Function &F = *I;
    if (!F.isDeclaration()) {
      runOnFunction(&instrumenter, F);
      Functions.insert(&F);
    }
  }
  llvm::errs() << "Instrumented " << instrumenter.getInstrumentedCnt() 
    << " instructions in total\n";

  string inputFileBasenameNoExt = getFileBaseName(inputFilename, false);
  if (outputFilename.empty()) {
    outputFilename = inputFileBasenameNoExt + "-instrumented.bc";
  }

  if (!saveModule(M.get(), outputFilename)) {
    errs() << "Failed to save the instrumented bitcode file to "
           << outputFilename << "\n";
    return 1;
  }
  errs() << "Saved the instrumented bitcode file to " << outputFilename << "\n";
  return 0;
}
