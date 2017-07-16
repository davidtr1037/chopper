#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <klee/klee.h>

#define BUG() \
{ \
    char *p = NULL; \
    *p = 0; \
}

#define SIZE 10

void target(char buf[SIZE], size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        buf[i] = i % 2;
    }
}

int main(int argc, char *argv[], char *envp[]) {
    char buf[SIZE];

    target(buf, sizeof(buf));
    if (buf[7] == 1) {
        BUG();
    }

    return 0;
}
