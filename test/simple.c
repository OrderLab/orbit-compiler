#include <stdio.h>
#include <stdlib.h>

// Global variable e
int e = 10;

int main() {
  int a = 3;
  int b = 5;
  int c;
  int *d;
  d = malloc(sizeof(int));
  *d = 3;
  if (b == 5) {
    a++;
  }
  // Accessing global variable e
  e++;
  printf("d is %p\n", d);
  c = a + b;
  return 0;
}
