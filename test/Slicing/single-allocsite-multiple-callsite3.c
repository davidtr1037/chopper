// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=foo,bar %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 4
// CHECK-STATES: KLEE: done: recovery states = 8
// CHECK-SLICES: KLEE: done: generated slices = 4
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

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

void bar(int **result, int *k) {
	int *i = malloc(sizeof *i);
    *result = i;
    if (*k > 0) {
    	**result = -1;
    } else {
    	**result = 123;
    }
}

int main(int argc, char *argv[], char *envp[]) {
	int k,j;
	klee_make_symbolic(&k, sizeof(k), "k");
	klee_make_symbolic(&j, sizeof(j), "j");

	int *a = NULL;
    foo(&a, &k);
    bar(&a, &j);
    if (*a > 3) {
    	printf("a gt 3\n");
    }

	return 0;
}
