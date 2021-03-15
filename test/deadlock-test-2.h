// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include <stdlib.h>
#include <iostream>

struct trx_t {
  int trx_id;
};

struct lock_t {
  int lock_id;
};

void *mem_heap_alloc(size_t size) __attribute__((annotate("heap_alloc"))) {
  return malloc(size);
}

class DeadlockChecker {
 public:
  DeadlockChecker();
  void check_and_resolve(trx_t *trx, lock_t *lock);
};

class RecLock {
 private:
  trx_t *m_trx;

 public:
  RecLock(trx_t *trx);
  lock_t *lock_alloc(size_t size);
  void add_to_waitq();
};
