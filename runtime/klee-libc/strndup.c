#include <string.h>
#include <stdlib.h>

char *strndup(const char *s1, size_t n) {
    char *s;

    n = strnlen(s1, n);
    if ((s = malloc(n + 1)) != NULL) {
        memcpy(s, s1, n);
        s[n] = 0;
    }

    return s;
}

