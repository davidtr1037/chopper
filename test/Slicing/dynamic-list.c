// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=insert_list %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 2
// CHECK-SLICES: KLEE: done: generated slices = 2
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdlib.h>
#include <assert.h>

typedef struct list {
    int key;
    struct list *next;
} mlist;

mlist *head;

void insert_list(int k){
    mlist *l = (mlist*) malloc(sizeof(mlist));
    l->key = k;
    head = l;
}

int main(int argc, char *argv[], char *envp[])
{
    mlist *temp;

    insert_list(2);
    assert(head->key == 2);
}
