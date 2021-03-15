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

struct target_t {
  trx_t *child;
};

void check_and_resolve(target_t *target) {}

void *mem_heap_alloc(size_t size) { return malloc(size); }

trx_t *ind_create_graph_create() { return (trx_t *)mem_heap_alloc(100); }

target_t *pars_complete_graph_for_exec(trx_t *trx) {
  target_t *target = (target_t *)mem_heap_alloc(100);
  target->child = trx;
  return target;
}

void row_create_index_for_mysql() {
  trx_t *trx = ind_create_graph_create();
  target_t *target = pars_complete_graph_for_exec(trx);
  check_and_resolve(target);
}
