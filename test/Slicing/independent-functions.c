// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f,g,h %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-B
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-Z
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-ANOT

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-STATES: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 2

// CHECK-A:a is 3
// CHECK-B:b is 2
// CHECK-Z:z is gt 3
// CHECK-ANOT:a is not 3

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
	o->y *= 2 + 1;
}

void h(point *o) {
	o->z += 1;
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 1; o.y = 1; o.z = 1;
    int a,b;

    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");

    f(&o);
    if (a == 3) {
    	printf("a is 3\n");
    } else {
    	printf("a is not 3\n");
    	g(&o);
    	if (o.x == 2) {
    		printf("b is 2\n");
    		h(&o);
    		if (o.z > 0) {
    			printf("z is gt 3\n");
    		}
    	} else {
    		printf("b is not 2\n");
    	}
    }

    return 0;
}
