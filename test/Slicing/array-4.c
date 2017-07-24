// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: not FileCheck %s -input-file=%t.out -check-prefix=CHECK-B

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-STATES: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2
// CHECK-A: Correct
// CHECK-B: Incorrect

#include <stdio.h>
#include <stdbool.h>

#include <klee/klee.h>

void f(short *buffer) {
    buffer[0] = 1000;
}

void g(short *buffer) {
    buffer[1] = 2000;
}

int main(int argc, char *argv[], char *envp[]) {
    short buffer[10] = {0,};

    f(buffer);
    g(buffer);

    if (buffer[0] == 1000) {
        printf("Correct\n");
    } else {
        printf("Incorrect\n");
    }

    return 0;
}
