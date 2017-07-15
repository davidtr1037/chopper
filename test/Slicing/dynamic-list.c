// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime -search=dfs -skip-functions=insert_list %t.bc > %t.out 2>&1

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
