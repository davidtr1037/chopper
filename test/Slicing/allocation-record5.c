// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %opt -mem2reg %t.bc -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -debug-only=basic -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 5
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

// CHECK-A: Correct
// CHECK-B: Incorrect

#include <stdio.h>
#include <stdlib.h>

#define MAGIX_X (100)
#define OBJECTS_COUNT (4)

typedef struct {
    int x;
} object_t;

object_t *g_objects[OBJECTS_COUNT];

void f() {
    for (unsigned int i = 0; i < OBJECTS_COUNT; i++) {
        object_t *o = malloc(sizeof(*o));
        g_objects[i] = o;
    }
}

void g() {
    for (unsigned int i = 0; i < OBJECTS_COUNT; i++) {
        g_objects[i]->x = MAGIX_X;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    f();
    g();
    object_t *o = g_objects[1];
    if (o->x == MAGIX_X) {
        printf("Correct\n");
    } else {
        printf("Incorrect\n");
    }

	return 0;
}
