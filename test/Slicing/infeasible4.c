// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=f,g,h %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-STATES -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-A
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-ANOT
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-B
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-BNOT
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-Z

// CHECK-PATHS: KLEE: done: completed paths = 7
// CHECK-STATES: KLEE: done: recovery states = 11
// CHECK-SLICES: KLEE: done: generated slices = 3
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 3

// CHECK-A:k is 3
// CHECK-ANOT:k is not
// CHECK-B:k is 2
// CHECK-BNOT:k is not 2
// CHECK-Z:z is gt 3

#include <stdio.h>
#include <assert.h>
#include <klee/klee.h>

typedef struct {
    int x;
    int y;
    int z;
} point;

void f(point *o, int *a) {
    if (*a > 0)
        o->x++;
    else
        o->x--;
}

void g(point *o, int *b) {
    if (*b > 0)
        o->y = o->x * 2 + 1;
    else
        o->y = o->x + 1;
}

void h(point *o, int *c) {
    if (*c > 0)
        o->z = o->y + 1;
}

int main(int argc, char *argv[], char *envp[]) {
    point o = {
        .x = 1,
        .y = 0,
        .z = 0
    };
    int a,b,c,k;

    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");
    klee_make_symbolic(&c, sizeof(b), "c");
    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o, &a);
    if (k == 3) {
        printf("k is 3\n");
    } else {
        printf("k is not 3\n");
        g(&o, &b);
        if (k == 2) {
            printf("k is 2\n");
            h(&o, &c);
            if (o.z > 3) {
                printf("z is gt 3\n");
            }
        } else {
            printf("k is not 2\n");
        }
    }

    return 0;
}
