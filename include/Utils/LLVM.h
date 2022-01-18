//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _UTILS_LLVM_H_
#define _UTILS_LLVM_H_

#include <memory>
#include <string>
#include <vector>

#include <llvm/Config/llvm-config.h>

#if LLVM_VERSION_MAJOR >= 4
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#else
#include <llvm/Bitcode/ReaderWriter.h>
#endif

#include <llvm/IR/Instructions.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/CallSite.h"

using namespace llvm;

std::unique_ptr<llvm::Module> parseModule(llvm::LLVMContext& context,
                                          std::string inputFile);

std::string demangleName(std::string);

// Operations on Functions
bool isConstructor(Value*);
std::string demangleFunctionName(Function*);
Function* getFunctionWithName(std::string name, Module& M);
std::vector<Function*> getFunctionsWithType(Type* type, Module& M);

// Operations on Instructions
bool isAccessingSameStructVar(const GetElementPtrInst* inst1,
                              const GetElementPtrInst* inst2);

// Printing
void printCallSite(Value* val);

std::tuple<Function *, Function *, unsigned>
extractCallerCallee(CallSite call, unsigned arg_no);

Constant *stripBitCastsAndAlias(Constant *c);

#endif /* _UTILS_LLVM_H_ */
