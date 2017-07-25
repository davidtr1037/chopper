// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-STATES: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

#include <stdio.h>
#include <klee/klee.h>

typedef struct {
  int x;
  int y;
  int z;
} point;

void f(point *o) {
  o->x++;
}

void g(point *o) {
  o->x = o->y * 2 + 1;
}

int main(int argc, char *argv[], char *envp[]) {
  point o;
  o.x = 1; o.y = 1; o.z = 1;

  f(&o);
  g(&o);
  if (o.x == 3) {
    printf("x is 3\n");
  } else {
    printf("x is not 3\n");
  }

  return 0;
}
