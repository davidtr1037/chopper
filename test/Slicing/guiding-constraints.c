// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-C

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-A: x = 1
// CHECK-B: x = 2
// CHECK-C: adding 1 guiding constraints

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
} point;

void f(point *p, int k) {
    if (k > 0) {
        p->x = 1;
    } else {
        p->x = 2;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    point o = {
        .x = 0,
        .y = 0
    };
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, k);
    if (k > 1) {
        printf("x = %d\n", o.x);
    }

    return 0;
}
