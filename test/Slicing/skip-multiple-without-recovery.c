// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-CORRECT
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-INCORRECT

// CHECK-CORRECT: correct
// CHECK-INCORRECT: incorrect
// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-STATES: KLEE: done: recovery states = 0
// CHECK-SLICES: KLEE: done: generated slices = 0
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>

#include <klee/klee.h>

typedef struct {
    int x;
    int y;
    int z;
} object_t;

void f(object_t *o) {
    o->x++;
}

void g(object_t *o) {
    o->y = o->x + 1;
}

int main(int argc, char *argv[], char *envp[]) {
    object_t o = {
        .x = 0,
        .y = 0,
        .z = 1
    };

    f(&o);
    g(&o);
    if (o.z) {
        printf("correct\n");
    } else {
        printf("incorrect\n");
    }

    return 0;
}
