// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=target %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <klee/klee.h>

#define BUG() \
{ \
    char *p = NULL; \
    *p = 0; \
}

#define SIZE 10

void target(char buf[SIZE], size_t size) {

    for (unsigned int i = 0; i < size; i++) {
        buf[i] = i % 2;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    char buf[SIZE];

    target(buf, sizeof(buf));
    if (buf[0] == 1) {
        BUG();
    }
    if (buf[1] == 1) {
        BUG();
    }

    return 0;
}
