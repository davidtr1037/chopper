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

#define MAGIX_X (100)
#define MAGIX_Y (200)

typedef struct {
    int x;
    int y;
} object_t;

void f(object_t *objects[], size_t count) {
    for (unsigned int i = 0; i < count; i++) {
        object_t *o = malloc(sizeof(*o));
        o->x = MAGIX_X;
        o->y = MAGIX_Y;
        objects[i] = o;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    object_t *objects[2];

    f(objects, 2);
    object_t *o = objects[0];
	if (o->x) {
        if (o->x == MAGIX_X) {
            printf("Correct\n");
        } else {
            printf("Incorrect\n");
        }
    }

	return 0;
}
