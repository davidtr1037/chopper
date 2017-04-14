/*===-- strnlen.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <string.h>

size_t strnlen(const char *str, size_t max) {
    const char *p = str;

    while (max && *p) {
        ++p;
        --max;
    }

    return p - str;
}
