// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=foo %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-RECOVERY: KLEE: done: recovery states = 0
// CHECK-SLICES: KLEE: done: generated slices = 0
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 0

#include <stdio.h>

int foo(int a) {
    return ++a;
}

int main(int argc, char *argv[], char *envp[]) {
    int ret = foo(10);
    int a;

    klee_make_symbolic(&a, sizeof(a), "a");
    if (a > 123) {
    	printf("First\n");
    } else {
    	printf("Second\n");
    }

	return 0;
}
