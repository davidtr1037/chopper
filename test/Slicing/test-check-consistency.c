// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error -search=dfs -skip-functions=f,g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-X
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-Y
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-OTHER

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-RECOVERY: KLEE: done: recovery states = 3
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

// CHECK-X: x is 987421
// CHECK-Y: y is 1974843
// CHECK-OTHER: other branch

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
    int z;
} point;

void f(point *o, int *k) {
	if (*k == 123)
		o->x++;
	else
		o->x = 987421;
}

void g(point *o) {
	o->y = o->x * 2 + 1;
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 1; o.y = 0; o.z = 0;
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, &k);
	g(&o);
    if (k == 3) {
    	printf("y is %d\n", o.y);
    	printf("x is %d\n", o.x);
    	if (o.x == 2) {
    		assert(0);
    	}
    } else {
    	printf("other branch\n");
    }

    return 0;
}
