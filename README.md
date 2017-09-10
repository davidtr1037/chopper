KLEE/CSE Project
=============================
An extension of KLEE (http://klee.github.io).

## Build
Build SVF (Pointer Analysis)
* https://github.com/davidtr1037/SVF/tree/master

Build DG (Static Slicing)
* https://github.com/davidtr1037/dg/tree/master

Build KLEE:
```
git checkout master
mkdir klee_build
cd klee_build
CXXFLAGS="-fno-rtti" cmake \
    -DENABLE_SOLVER_STP=ON \
    -DENABLE_POSIX_RUNTIME=ON \
    -DENABLE_KLEE_UCLIBC=ON \
    -DKLEE_UCLIBC_PATH=<KLEE_UCLIBC_DIR> \
    -DLLVM_CONFIG_BINARY=<LLVM_CONFIG_PATH> \
    -DENABLE_UNIT_TESTS=OFF \
    -DKLEE_RUNTIME_BUILD_TYPE=Release+Asserts \
    -DENABLE_SYSTEM_TESTS=ON \
    -DSVF_ROOT_DIR=<SVF_PROJECT_DIR> \
    -DDG_ROOT_DIR=<DG_PROJECT_DIR> \
    <KLEE_ROOT_DIR>
make
```

Notes:
* Use llvm-config from the CMake build (LLVM 3.4)

## Usage Example
Let's look at the following program:
```C
#include <stdio.h>

#include <klee/klee.h>

typedef struct {
    int x;
    int y;
} object_t;

void f(object_t *o) {
    o->x = 0;
    o->y = 0;
}

int main(int argc, char *argv[]) {
    object_t o;
    int k;

    klee_make_symbolic(&k, sizeof(k), "k");

    f(&o);
    if (k > 0) {
        printf("%d\n", o.x);
    } else {
        printf("%d\n", o.y);
    }

    return 0;
}
```

Compile the program:
```
clang -c -g -emit-llvm main.c -o main.bc
opt -mem2reg main.bc -o main.bc (required for better pointer analysis)
```

Run KLEE (static analysis related debug messages are written to stdout):
```
klee -libc=klee -search=dfs -slice=f main.bc
```

## Options
### Skipping Functions
The skipped functions are set using the following option:
```
-skip-functions=<function1>[:line1/line2/...],<function2>[:line1/line2/...],...
```
### Inlining
In some cases, inlining can improve the precision of static analysis.
Functions can be inlined using the following option:
```
-inline=<function1>,<function2>,...
```
### Debugging
More verbose debug messages can be produced using the following option:
```
-debug-only=basic
```

## Notes:
* Currently, the supported search heuristics are: dfs, bfs, random-state, nurs:covnew.
* When using klee-libc, some files (memcpy.c, memset.c) should be recompiled with `-O1` to avoid vector instructions.
