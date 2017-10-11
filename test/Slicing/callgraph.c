// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=nurs:patch -target-location=callgraph.c:15 -target-function=h %t.bc > %t.out 2>&1

#include <stdio.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
    int z;
} point;

void h(point *o) {
	o->z += 1;
}

void g(point *o) {
	o->y *= 2 + 1;
	h(o);
}

void f(point *o) {
	o->x++;
	g(o);
}

int main(int argc, char *argv[], char *envp[]) {
    point o;
    o.x = 1; o.y = 1; o.z = 1;
    int a,b;

    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");

    if (a == 3) {
    	printf("a is 3\n");
    } else {
    	printf("a is not 3\n");
    	if (b == 2) {
    		printf("b is 2\n");
    		if (o.z > 0) {
    			printf("z is gt 3\n");
                f(&o);
    		}
    	} else {
    		printf("b is not 2\n");
    	}
    }

    return 0;
}
