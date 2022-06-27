#include <stdio.h>

#include "compiler.h"
#include "scanner.h"

void compile(const char *source) {
    Scanner scanner; scanner_init(&scanner, source);
    int line = -1;

    for (;;) {
        Token token = scanner_scan_token(&scanner);

        if (token.line != line) {
            line = token.line;
            printf("%4d ", line);
        }
        else {
            printf("   | ");
        }

        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) break;
    }
}
