// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime -search=dfs -skip-functions=foo %t.bc > %t.out 2>&1

#include <assert.h>

void foo(int *a)
{
	++a;
	*a = 8;
}

int main(int argc, char *argv[], char *envp[])
{
	int a[2] = {0,1};
	foo(a);
	assert(a[1] == 8);
	return 0;
}
