#include <string.h>

#include "scanner.h"

void scanner_init(Scanner *scanner, const char *source) {
    scanner->line = 1;
    scanner->start = source;
    scanner->current = source;
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') || c == '_';
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
            case '\r': {
                advance(scanner);
                break;
            }
            case '\n': {
                scanner->line++;
                advance(scanner);
                break;
            }
            case '/': {
                if (peek_next(scanner) != '/')
                    return;

                while (peek(scanner) != '\n' && !is_at_end(scanner))
                    advance(scanner);
                break;
            }
            default:
                return;
        }
    }
}

static Token scan_string(Scanner *scanner, char quote) {
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

static Token scan_number(Scanner *scanner) {
    while (is_digit(peek(scanner)))
        advance(scanner);

    if (peek(scanner) == '.' && is_digit(peek_next(scanner))) {
        do {
            advance(scanner);
        }
        while (is_digit(peek(scanner)));
    }

    return make_token(scanner, TOKEN_NUMBER);
}

static TokenType check_keyword(Scanner *scanner, int start, int length,
                               const char *rest, TokenType type) {
    if (scanner->current - scanner->start == start + length &&
            memcmp(scanner->start + start, rest, length) == 0)
        return type;

    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Scanner *scanner) {
    switch (scanner->start[0]) {
        case 'a': return check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
        case 'c': return check_keyword(scanner, 1, 4, "lass", TOKEN_CLASS);
        case 'e': return check_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'a': return check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
                    case 'n': return TOKEN_FN;
                }
            }
            break;
        case 'i': return check_keyword(scanner, 1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(scanner, 1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return check_keyword(scanner, 1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'h': return check_keyword(scanner, 2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'l': return check_keyword(scanner, 1, 2, "et", TOKEN_LET);
        case 'w': return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token scan_identifier(Scanner *scanner) {
    while (is_alpha(peek(scanner)) || is_digit(peek(scanner)))
        advance(scanner);

    return make_token(scanner, identifier_type(scanner));
}

Token scanner_scan_token(Scanner *scanner) {
    skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (is_at_end(scanner))
        return make_token(scanner, TOKEN_EOF);

    char c = advance(scanner);

    if (is_digit(c)) return scan_number(scanner);
    if (is_alpha(c)) return scan_identifier(scanner);

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

        case '\'':
        case '"' :
            return scan_string(scanner, c);
    }

    return error_token(scanner, "unexpected character");
}
