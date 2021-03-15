// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "deadlock-detector.h"
#include <stdlib.h>
#include <iostream>

static inline void* mem_heap_alloc(size_t size) { return malloc(size); }

lock_t* RecLock::lock_table_create(dict_table_t* table) {
  lock_t* lock;
  int type;

  if (type == 0) {
    lock = table->lock;
  } else {
    lock = static_cast<lock_t*>(mem_heap_alloc(100));
  }

  return lock;
}

bool RecLock::lock_table_enqueue_waiting(int mode, dict_table_t* table,
                                         float* thr) {
  trx_t* trx;
  lock_t* lock;

  lock = lock_table_create(table);
  const trx_t* victim_trx = DeadlockChecker::check_and_resolve(lock, trx);

  if (victim_trx != 0) {
    return true;
  }
  return false;
}

DeadlockChecker::DeadlockChecker(const trx_t* trx, const lock_t* wait_lock,
                                 int mark_start) {
  m_lock = (lock_t*)malloc(100);
}

void DeadlockChecker::search() {
  if (m_lock != NULL) {
    return;
  }
}

trx_t* DeadlockChecker::check_and_resolve(const lock_t* lock, trx_t* trx) {
  trx_t* victim_trx;
  int s_lock_mark_counter = 0;
  DeadlockChecker checker(trx, lock, s_lock_mark_counter);

  if (true) {
    checker.search();
  }
  return victim_trx;
}

lock_t* RecLock::lock_alloc(trx_t* trx, int index, int mode, int rec_id,
                            int size) {
  lock_t* lock;
  if (true) {
    lock = static_cast<lock_t*>(mem_heap_alloc(100));
  } else {
    lock = trx->lock;
  }
  return lock;
}

lock_t* RecLock::create(trx_t* trx, bool owns_trx_mutex, bool add_to_hash,
                        const lock_prdt_t* prdt) {
  lock_t* lock = lock_alloc(trx, m_index, m_mode, m_rec_id, m_size);
  return lock;
}

void RecLock::add_to_waitq(const lock_t* wait_for, const lock_prdt_t* prdt) {
  lock_t* lock = create(m_trx, true, true, prdt);
  deadlock_check(lock);
}

void RecLock::deadlock_check(lock_t* lock) {
  const trx_t* victim_trx = DeadlockChecker::check_and_resolve(lock, m_trx);
}
