#include <string.h>

#include "scanner.h"

void scanner_init(Scanner *scanner, const char *source) {
    scanner->line = 1;
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

static char peek(Scanner *scanner) {
    return *scanner->current;
}

static char peek_next(Scanner *scanner) {
    if (is_at_end(scanner))
        return '\0';

    return scanner->current[1];
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

static void skip_whitespace(Scanner *scanner) {
    for (;;) {
        switch (peek(scanner)) {
            case ' ' :
            case '\t':
            case '\r':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            case '/':
                if (peek_next(scanner) != '/')
                    return;

                while (peek(scanner) != '\n' && !is_at_end(scanner))
                    advance(scanner);
                break;
            default:
                return;
        }
    }
}

Token scan_string(Scanner *scanner, char quote) {
    while (peek(scanner) != quote && !is_at_end(scanner)) {
        if (peek(scanner) == '\n')
            scanner->line++;

        advance(scanner);
    }

    if (is_at_end(scanner))
        return error_token(scanner, "unterminated string");

    advance(scanner);
    return make_token(scanner, TOKEN_STRING);
}

Token scanner_scan_token(Scanner *scanner) {
    char c;
    skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (is_at_end(scanner))
        return make_token(scanner, TOKEN_EOF);

    switch (c = advance(scanner)) {
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

        case '\'':
        case '"' :
            return scan_string(scanner, c);
    }

    return error_token(scanner, "unexpected character");
}
