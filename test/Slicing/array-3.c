// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-STATES: KLEE: done: recovery states = 10
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1
// CHECK-A: Correct
// CHECK-B: Incorrect

#include <stdio.h>
#include <stdbool.h>

#include <klee/klee.h>

void f(char *buffer, unsigned int size) {
    memset(buffer, 7, size);
}

int main(int argc, char *argv[], char *envp[]) {
    char buffer[10] = {0,};

    f(buffer, sizeof(buffer));

    bool valid = true;
    for (unsigned int i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] != 7) {
            valid = false;
            break;
        }
    }

    if (valid) {
        printf("Correct\n");
    } else {
        printf("Incorrect\n");
    }

    return 0;
}
