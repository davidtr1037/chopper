// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=bfs -exit-on-error-type Assert -error-location=ExitOnErrorLocation.c:11/13 %t1.bc 2>&1

#include <assert.h>
#include <klee/klee.h>

int main() {
  int x = klee_int("x");
  if (x < 42) {
	assert(0);
  } else if (x > 100) {
	assert(0);
  }
  while(1) { }
  return 0;
}
