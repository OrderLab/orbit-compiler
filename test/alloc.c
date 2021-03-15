#include <stdio.h>
#include <stdlib.h>

struct item {
  int value;
  struct item *next;
};

int * sum_items(struct item *head) {
  int *sum = (int *) malloc(4);
  *sum = 0;
  struct item *p = head;
  while (p != NULL) {
    *sum = *sum + p->value;
    p = p->next;
  }
  return sum;
}

int foo(int data) {
  struct item *item1 = (struct item *) malloc(sizeof(struct item));
  item1->value = data;

  struct item *item2 = (struct item *) malloc(sizeof(struct item));
  item2->value = data * 2;
  item1->next = item2;

  struct item *item3 = (struct item *) malloc(sizeof(struct item));
  item3->value = data * 3;
  item2->next = item3;
  item3->next = NULL;

  int *sum = sum_items(item1);
  int ret = *sum;

  free(sum);
  free(item1);
  free(item2);
  free(item3);

  return ret;
}

int main(void) {
  printf("foo(5)=%d\n", foo(5));
  printf("foo(15)=%d\n", foo(15));
  printf("foo(20)=%d\n", foo(20));
}

