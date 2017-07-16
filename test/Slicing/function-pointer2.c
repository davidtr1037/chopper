// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime -search=dfs -skip-functions=call %t.bc > %t.out 2>&1

#include <assert.h>

int glob;

void setglob(void)
{
	glob = 8;
}

void setglob2(void)
{
	glob = 13;
}

void (*funcarray[10])(void) = {setglob, setglob2};

void call(void (**funcarray)(void), int idx)
{
	funcarray[idx]();
}

int main(int argc, char *argv[], char *envp[])
{
	call(funcarray, 1);
	assert(glob == 13);
	return 0;
}
