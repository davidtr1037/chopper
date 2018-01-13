// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=foo %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 4
// CHECK-STATES: KLEE: done: recovery states = 11
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 3

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

void foo(int **result, int *k) {
	int *i = malloc(sizeof *i);
    *result = i;
    if (*k > 0) {
    	**result = 0;
    } else {
    	**result = 987641;
    }
}

int main(int argc, char *argv[], char *envp[]) {
	int k,j;
	klee_make_symbolic(&k, sizeof(k), "k");
	klee_make_symbolic(&j, sizeof(j), "j");

	int *a = NULL;
    foo(&a, &k);
    if (*a < 3) {
    	printf("got a\n");
    }
    foo(&a, &j);
    if (*a > 3) {
    	printf("a gt 3\n");
    }

	return 0;
}
