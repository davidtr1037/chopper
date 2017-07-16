// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime -search=dfs -skip-functions=f %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A -check-prefix=CHECK-B -check-prefix=CHECK-Z -check-prefix=CHECK-ANOT -check-prefix=CHECK-BNOT

// CHECK-PATHS: KLEE: done: completed paths = 3
// CHECK-STATES: KLEE: done: recovery states = 4
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-A:a is 3
// CHECK-B:b is 2
// CHECK-Z:z is gt 3
// CHECK-ANOT:a is not 3
// CHECK-BNOT:b is not 2

#include <stdio.h>
#include <assert.h>
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
	o->y = o->x * 2 + 1;
}

void h(point *o) {
	o->z = o->y + 1;
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 1; o.y = 0; o.z = 0;
    int a,b,c;

    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");
    klee_make_symbolic(&c, sizeof(c), "c");

    f(&o);
    if (a == 3) {
    	printf("a is 3\n");
    } else {
    	printf("a is not 3\n");
    	g(&o);
    	if (b == 2) {
    		printf("b is 2\n");
    		h(&o);
    		if (o.z > 3) {
    			printf("z is gt 3\n");
    		}
    	} else {
    		printf("b is not 2\n");
    	}
    }

    return 0;
}
