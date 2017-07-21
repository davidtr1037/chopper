// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-B
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-C

// CHECK-PATHS: KLEE: done: completed paths = 3
// CHECK-STATES: KLEE: done: recovery states = 3
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

// CHECK-A: x = 1
// CHECK-B: x = 2
// CHECK-C: y = 7
// CHECK-D: adding 0 guiding constraints
// CHECK-E: adding 2 guiding constraints

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

void g(point *p) {
    p->y = 7;
}

int main(int argc, char *argv[], char *envp[]) {
    point o = {
        .x = 0,
        .y = 0
    };
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, k);
    printf("x = %d\n", o.x);

    if (k > 1) {
        g(&o);
        printf("y = %d\n", o.y);
    }

    return 0;
}
