// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %opt -mem2reg %t.bc -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 4
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

// CHECK-A: Correct
// CHECK-B: Incorrect

#include <stdio.h>
#include <stdlib.h>

#define MAGIX_X (100)

typedef struct {
    int x;
} object_t;

object_t *g_object = NULL;

void f() {
    if (g_object == NULL) {
        g_object = malloc(sizeof(*g_object));
        g_object->x = MAGIX_X;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    f();
    f();
    if (g_object->x == MAGIX_X) {
        printf("Correct\n");
    } else {
        printf("Incorrect\n");
    }

	return 0;
}
