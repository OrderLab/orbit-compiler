// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "deadlock-test-2.h"

DeadlockChecker::DeadlockChecker() {}

void DeadlockChecker::check_and_resolve(trx_t *trx, lock_t *lock)
    __attribute__((annotate("target"))) {
  // Do work
  return;
}

RecLock::RecLock(trx_t *trx) { m_trx = trx; }

lock_t *RecLock::lock_alloc(size_t size) {
  return static_cast<lock_t *>(mem_heap_alloc(size));
}

void RecLock::add_to_waitq() {
  lock_t *lock = lock_alloc(100);
  DeadlockChecker *checker = new DeadlockChecker();
  checker->check_and_resolve(m_trx, lock);
}

int main() {
  trx_t *trx = static_cast<trx_t *>(mem_heap_alloc(100));
  RecLock *rec = new RecLock(trx);
  rec->add_to_waitq();
  return 0;
}