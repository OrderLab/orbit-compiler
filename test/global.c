#include <stdio.h>
#include <stdlib.h>

struct lock_t {
  int lock_id;
};

struct trx_t {
  int trx_id;
  struct lock_t *lock;
};

// Global Variable
static struct trx_t *trx;

static int gx = 10;
/* const int gx = 10; */

struct lock_t *create_lock() {
  int size;
  size = (int)sizeof(struct lock_t);
  struct lock_t *lock = malloc(size);
  return lock;
}

void create_trx() {
  trx = malloc(sizeof(struct trx_t));
  trx->lock = malloc(sizeof(struct lock_t));
}

void check_and_resolve(struct lock_t *lock) {
/* void check_and_resolve(struct lock_t *lock, struct trx_t *trx) { */
  /* struct trx_t *victim_trx; */
  int check = gx;
  unsigned heap_no;

  if (check) {
    // Perform operations on lock and trx
    for (int i = 0; i < 10; i++) {
      lock->lock_id = 100;
      trx->trx_id = 100;
      check += (trx->lock == lock);
    }
  }

  return;
}

int main() {
  struct lock_t *lock = create_lock();
  create_trx();
  check_and_resolve(lock);
  // This still does not work.
  /* check_and_resolve(lock, trx); */
  return 0;
}
