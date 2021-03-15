// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _ALLOC_INSTRUMENT_H_
#define _ALLOC_INSTRUMENT_H_

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"

#include "Utils/LLVM.h"

#include <map>
#include <set>
#include <string>

namespace llvm {

namespace slicing {
class Slice;
}

namespace instrument {
extern unsigned int AllocVarGuidStart;

/* String constants for the names of tracker functions defined in
 * runtime/gobj_tracker.h
 */

inline StringRef getRuntimeHookInitName() {
  return "__orbit_gobj_tracker_init";
}

inline StringRef getRuntimeHookName() { return "__orbit_alloc_gobj"; }

inline StringRef getTrackDumpHookName() { return "__orbit_gobj_tracker_dump"; }

inline StringRef getTrackHookFinishName() {
  return "__orbit_gobj_tracker_finish";
}

class AllocInstrumenter {
 public:
  // by default we will use our lightweight runtime library for tracking
  // setting use_printf to true will use printf for tracking
  AllocInstrumenter(bool use_printf = true)
      : _initialized(false),
        _instrument_cnt(0),
        _track_with_printf(use_printf) {}

  bool initHookFuncs(Module *M, LLVMContext &context);

  // instrument a call to hook func before an instruction.
  // this instruction must be an allocation call instruction
  bool instrumentInstr(Instruction *instr);

  uint32_t getInstrumentedCnt() { return _instrument_cnt; }

 protected:
  bool _initialized;
  uint32_t _instrument_cnt;
  // instrument printf to track alloc, useful for testing
  bool _track_with_printf;

  Function *_main;
  Function *_printf_func;

  Function *_track_gobj_func;
  Function *_tracker_init_func;
  Function *_tracker_dump_func;
  Function *_tracker_finish_func;

  std::map<uint64_t, Instruction *> _guid_hook_point_map;
  std::map<Instruction *, uint64_t> _hook_point_guid_map;

  IntegerType *_I32Ty;
  PointerType *_I8PtrTy;
};

}  // namespace instrument
}  // namespace llvm

#endif /* _ALLOC_INSTRUMENT_H_ */
