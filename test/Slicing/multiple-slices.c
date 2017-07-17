// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 4
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
} point;

void f(point *o, int k) {
    if (k > 0) {
    	o->x++;
    } else {
        o->y++;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 0; o.y = 0;
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, k);
    if (o.x > 0) {
    	printf("x greater than zero\n");
        if (o.y > 0) {
        	assert(0);
        }
    }
    if (o.y > 0) {
    	printf("y greater than zero\n");
    	if (o.x > 0) {
    		assert(0);
    	}
    }

    return 0;
}
