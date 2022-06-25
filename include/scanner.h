#ifndef clox_scanner_h
#define clox_scanner_h

#include "common.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

void scanner_init(Scanner *scanner, const char *source);

#endif
