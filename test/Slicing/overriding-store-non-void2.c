// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
    int w;
    int z;
} point;

point f() {
	point o;
	o.x = 0; o.y = 0; o.w = 0; o.z = 0;
	o.x++;
	o.y++;
	return o;
}

int main(int argc, char *argv[], char *envp[]) {
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    point o = f();
    o.x = 876; // overriding store, but too complex to catch in the pass
    if (k > 0) {
    	printf("x: %d\n", o.x);
    }

    return 0;
}
