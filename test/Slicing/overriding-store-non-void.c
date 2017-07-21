// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 0
// CHECK-SLICES: KLEE: done: generated slices = 0
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

int f(int k) {
  return ++k;
}

int main(int argc, char *argv[], char *envp[]) {
    int k;
    klee_make_symbolic(&k, sizeof(k), "k");

    int ret = f(k);
    ret = 876;
    if (k > 0) {
    	printf("ret: %d\n", ret);
    }

    return 0;
}
