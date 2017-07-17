// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-T
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-F

// CHECK-T: True branch
// CHECK-F: False branch
// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>

#include <klee/klee.h>

typedef struct {
    int x;
    int y;
} object_t;

void f(object_t *o) {
    o->x = 0;
    o->y = 0;
}

int main(int argc, char *argv[], char *envp[]) {
    object_t o;
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o);
    if (k > 0) {
        printf("True branch %d\n", o.x);
    } else {
        printf("False branch %d\n", o.y);
    }

    return 0;
}
