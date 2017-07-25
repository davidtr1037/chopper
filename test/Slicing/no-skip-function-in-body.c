// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -search=dfs -skip-functions=foo2 %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-ERROR

// CHECK-ERROR: KLEE: ERROR: skip-function option: 'foo2' not found in module

#include <stdlib.h>
#include <assert.h>

void foo(int **result)
{
	int *i = malloc(sizeof *i);
    *result = i;
}

int main(int argc, char *argv[], char *envp[])
{
	int *a = NULL;
    foo(&a);
	*a = 3;
	assert(*a == 3);

	return 0;
}
