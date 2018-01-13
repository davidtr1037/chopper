// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 4
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

// CHECK-A: x = 0, y = 1
// CHECK-B: x = 1, y = 2

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
} point;

void f(point *p, int k) {
    if (k == 0) {
        p->x = 0;
    } else {
        p->x = 1;
    }
}

void g(point *p) {
    p->y = p->x + 1;
}

int main(int argc, char *argv[], char *envp[]) {
    point o = {
        .x = 0,
        .y = 0
    };
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, k);
    g(&o);
    printf("x = %d, y = %d\n", o.x, o.y);

    return 0;
}
