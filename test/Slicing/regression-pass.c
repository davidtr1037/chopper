// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -search=dfs -skip-functions=foo %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PATHS -check-prefix=CHECK-RECOVERY -check-prefix=CHECK-SLICES -check-prefix=CHECK-SNAPSHOTS
// RUN: FileCheck %s -input-file=%t.out -check-prefix=CHECK-PRINTF -check-prefix=CHECK-PTA

// CHECK-PATHS: KLEE: done: completed paths = 2
// CHECK-RECOVERY: KLEE: done: recovery states = 1
// CHECK-SLICES: KLEE: done: generated slices = 1
// CHECK-SNAPSHOTS: KLEE: done: created snapshots = 1

// CHECK-PRINTF: KLEE: WARNING ONCE: calling external: printf(
// CHECK-PTA: INFO: Points-to analysis took

int foo(int x) {
  return x + 1;
}

int main() {
  int k;
  klee_make_symbolic(&k, sizeof(k), "k");

  int r = foo(7);
  if (k) {
    printf("%d\n", r);
  } else {
    printf("\n");
  }
}
