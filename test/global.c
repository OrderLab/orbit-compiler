#include <stdio.h>
#include <stdlib.h>

struct lock_t {
  int lock_id;
};

struct trx_t {
  int trx_id;
};

// Global Variable
static struct trx_t *trx;

struct lock_t *create_lock() {
  int size;
  size = (int)sizeof(struct lock_t);
  struct lock_t *lock = malloc(size);
  return lock;
}

void create_trx() {
  trx = malloc(sizeof(struct trx_t));
}

void check_and_resolve(struct lock_t *lock) {
  struct trx_t *victim_trx;
  int check = 0;
  unsigned heap_no;

  if (check) {
    // Perform operations on lock and trx
    for (int i = 0; i < 10; i++) {
      lock->lock_id = 100;
      trx->trx_id = 100;
    }
  }

  return;
}

int main() {
  struct lock_t *lock = create_lock();
  check_and_resolve(lock);
  return 0;
}
