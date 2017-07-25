// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %opt -mem2reg %t.bc -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-A: Correct
// CHECK-B: Incorrect

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x;
} object_t;

void f(int x, object_t **result) {
    if (!result) {
        return;
    }

    object_t *o = malloc(sizeof(*o));
    o->x = x;

    *result = o;
}

int main(int argc, char *argv[], char *envp[]) {
    object_t *o;

    f(7, &o);
	if (o) {
        if (o->x == 7) {
            printf("Correct\n");
        } else {
            printf("Incorrect\n");
        }
    }

	return 0;
}
