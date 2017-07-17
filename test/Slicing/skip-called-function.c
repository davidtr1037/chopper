// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=g %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-B
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-Y
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-ANOT
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-BNOT

// CHECK-PATHS: KLEE: done: completed paths = 3
// CHECK-STATES: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-A:a is 3
// CHECK-B:b is 2
// CHECK-Y:y is gt 3
// CHECK-ANOT:a is not 3
// CHECK-BNOT:b is not 2

#include <stdio.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
    int z;
} point;

void f(point *o) {
	o->x++;
}

void g(point *o) {
	f(o);
	o->y = o->x * 2 + 1;
}

void h(point *o) {
	g(o);
	o->z = 7;
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 1; o.y = 0; o.z = 0;
    int a,b;

    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");

    if (a == 3) {
    	printf("a is 3\n");
    } else {
    	printf("a is not 3\n");
    	if (b == 2) {
    		printf("b is 2\n");
    		h(&o);
    		if (o.y > 3) {
    			printf("y is gt 3\n");
    		}
    	} else {
    		printf("b is not 2\n");
    	}
    }

    return 0;
}
