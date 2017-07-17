// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-ANOT

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-A: a is gt 0
// CHECK-ANOT: a is not gt 0

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
} point;

void f(point *o, int *a) {
	if (*a > 0) {
		printf("incrementing x\n");
		o->x++;
	}
	else {
		printf("decrementing x\n");
		o->x--;
	}
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 1;
    int a;

    klee_make_symbolic(&a, sizeof(a), "a");
    int c = a;

    f(&o, &a);
    if (c > 0) {
    	printf("a is gt 0\n");
    	if (o.x == 0) {
    		assert(0);
    	}
    } else {
    	printf("a is not gt 0\n");
    	if (o.x == 2) {
    		assert(0);
    	}
    }

    return 0;
}
