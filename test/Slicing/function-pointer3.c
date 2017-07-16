// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=foo %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

#include <stdio.h>
#include <assert.h>

int glob;

int *setglob(void)
{
	glob = 23;
    return NULL;
}

void foo(int *(f)(void), int **result)
{
	*result = f();
}

int main(int argc, char *argv[], char *envp[])
{
	int *p;
	foo(setglob, &p);
	assert(glob == 23);
	return 0;
}
