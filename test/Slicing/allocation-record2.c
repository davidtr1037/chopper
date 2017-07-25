// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %opt -mem2reg %t.bc -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 3
// CHECK-SLICES: KLEE: done: generated slices = 3
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-A: Correct
// CHECK-B: Incorrect

#include <stdio.h>
#include <stdlib.h>

#define MAGIX_X (100)
#define MAGIX_Y (200)

typedef struct {
    int x;
    int y;
} object_t;

void f(int x, int y, object_t **result) {
    if (!result) {
        return;
    }

    object_t *o = malloc(sizeof(*o));
    o->x = x;
    o->y = y;

    *result = o;
}

int main(int argc, char *argv[], char *envp[]) {
    object_t *o;

    f(MAGIX_X, MAGIX_Y, &o);
	if (o) {
        if (o->x == MAGIX_X && o->y == MAGIX_Y) {
            printf("Correct\n");
        } else {
            printf("Incorrect\n");
        }
    }

	return 0;
}
