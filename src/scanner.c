#include "scanner.h"

void scanner_init(Scanner *scanner, const char *source) {
    scanner->line = 0;
    scanner->start = source;
    scanner->current = source;
}
