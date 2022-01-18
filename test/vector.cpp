#include <vector>
#include <cstdio>
#include <cstdlib>

struct lock_t {
    int x;
};

struct trx_t {
    std::vector<lock_t*> locks;
};

void check(trx_t *trx, lock_t *lock) {
    printf("%d\n", lock->x);
}

int main() {
    trx_t trx;
    trx.locks.push_back((lock_t*)malloc(sizeof(lock_t)));
    lock_t *lock = trx.locks[0];
    check(&trx, lock);
    return 0;
}
