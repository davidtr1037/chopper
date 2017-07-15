// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime -search=dfs -skip-functions=call %t.bc > %t.out 2>&1

#include <assert.h>

int glob;

void setglob(void)
{
	glob = 8;
}

void (*funcarray[10])(void) = {setglob};

void call(void (**funcarray)(void), int idx)
{
	funcarray[idx]();
}

int main(int argc, char *argv[], char *envp[])
{
	call(funcarray, 0);
	assert(glob == 8);
	return 0;
}
