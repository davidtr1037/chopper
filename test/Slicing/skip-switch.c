// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-B
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-C

// CHECK-A: x is 10
// CHECK-B: x is 20
// CHECK-C: adding 1 guiding constraints
// CHECK-PATHS: KLEE: done: completed paths = 3
// CHECK-STATES: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>

#include <klee/klee.h>

typedef struct {
    int x;
    int y;
} object_t;

void f(object_t *o, int k) {
    switch (k) {
    case 0:
        o->x = 0;
        break;

    case 1:
        o->x = 10;
        break;

    default:
        o->x = 20;
        break;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    object_t o;
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, k);
    if (k > 0) {
        printf("x is %d\n", o.x);
    }

    return 0;
}

