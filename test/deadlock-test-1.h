// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include <iostream>

struct lock_t {
  int id;
};

struct trx_t {
  int id;
  lock_t *lock;
};

struct dict_table_t {
  struct lock_t *lock;
};

struct lock_prdt_t {};

class DeadlockChecker {
 public:
  DeadlockChecker(const trx_t *, const lock_t *, int);
  void search();
  static trx_t *check_and_resolve(const lock_t *, trx_t *);

 private:
  lock_t *m_lock;
};

class RecLock {
 private:
  trx_t *m_trx;
  int m_mode;
  int m_index;
  int m_rec_id;
  int m_size;

 public:
  lock_t *lock_alloc(trx_t *, int, int, int, int);
  lock_t *create(trx_t *, bool, bool, const lock_prdt_t *);
  void add_to_waitq(const lock_t *, const lock_prdt_t *);
  void deadlock_check(lock_t *);
  lock_t *lock_table_create(dict_table_t *);
  bool lock_table_enqueue_waiting(int, dict_table_t *, float *);
};