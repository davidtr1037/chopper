// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-STATES: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1
// CHECK-A: True branch
// CHECK-B: False branch

#include <stdio.h>

#include <klee/klee.h>

void f(char *s) {
    s[0] = 7;
}

int main(int argc, char *argv[], char *envp[]) {
    char array[10] = {0,};

    f(array);
    if (array[0] == 7) {
        printf("True branch\n");
    } else {
        printf("False branch\n");
    }

    return 0;
}
