// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=call %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS

// CHECK-PATHS: KLEE: done: completed paths = 1
// CHECK-RECOVERY: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

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
