#include <string.h>

#include "scanner.h"

void scanner_init(Scanner *scanner, const char *source) {
    scanner->line = 0;
    scanner->start = source;
    scanner->current = source;
}

static bool is_at_end(Scanner *scanner) {
    return *scanner->current == '\0';
}

static char advance(Scanner *scanner) {
    return *(scanner->current++);
}

static bool match(Scanner *scanner, char expected) {
    if (is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;

    scanner->current++;
    return true;
}

static Token make_token(Scanner *scanner, TokenType type) {
    return (Token){
        .type   = type,
        .line   = scanner->line,
        .start  = scanner->start,
        .length = (int)(scanner->current - scanner->start)
    };
}

static Token error_token(Scanner *scanner, const char *message) {
    return (Token){
        .type   = TOKEN_ERROR,
        .line   = scanner->line,
        .start  = message,
        .length = (int)strlen(message)
    };
}

Token scanner_scan_token(Scanner *scanner) {
    scanner->start = scanner->current;

    if (is_at_end(scanner))
        return make_token(scanner, TOKEN_EOF);

    char c = advance(scanner);
    switch (c) {
        case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
        case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
        case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
        case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
        case ';': return make_token(scanner, TOKEN_SEMICOLON);
        case '.': return make_token(scanner, TOKEN_DOT);
        case ',': return make_token(scanner, TOKEN_COMMA);
        case '+': return make_token(scanner, TOKEN_PLUS);
        case '-': return make_token(scanner, TOKEN_MINUS);
        case '*': return make_token(scanner, TOKEN_STAR);
        case '/': return make_token(scanner, TOKEN_SLASH);

        case '!':
            return make_token(
                scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(
                scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(
                scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(
                scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return error_token(scanner, "unexpected character");
}
