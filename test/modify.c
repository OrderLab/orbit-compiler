#include <stdio.h>
#include <stdlib.h>

struct trx_t {
    int *a;
};

void modify(struct trx_t *t) {
    t->a = (int*)malloc(sizeof(int));
}

/* void check(int x, struct trx_t *t) { */
void check(int x, void *t) {
    (void)x;
    printf("%d\n", *((struct trx_t*)t)->a);
}

int main() {
    struct trx_t *t = malloc(sizeof(struct trx_t));
    modify(t);
    check(0, t);
    return 0;
}
