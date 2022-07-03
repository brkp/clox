#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token prev;
    Token curr;

    bool had_error;
    bool panic_mode;

    Chunk *compiling_chunk;
} Parser;

static void error_at(Parser *parser, Token *token, const char *message) {
    if (parser->panic_mode)
        return;

    parser->had_error = true;
    parser->panic_mode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    switch (token->type) {
        case TOKEN_EOF:
            fprintf(stderr, " at end"); break;
        case TOKEN_ERROR:
            break;
        default:
            fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
}

static void error_at_current(Parser *parser, const char *message) {
    error_at(parser, &parser->curr, message);
}

static void advance(Scanner *scanner, Parser *parser) {
    parser->prev = parser->curr;

    for (;;) {
        parser->curr = scanner_scan_token(scanner);
        if (parser->curr.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->curr.start);
    }
}

static void consume(Scanner *scanner, Parser *parser, TokenType type, const char *message) {
    if (parser->curr.type == type) {
        advance(scanner, parser);
        return;
    }

    error_at_current(parser, message);
}

static void emit_byte(Parser *parser, uint8_t byte) {
    chunk_push(parser->compiling_chunk, byte, parser->prev.line);
}

static void emit_return(Parser *parser) {
    emit_byte(parser, OP_RETURN);
}

bool compile(const char *source, Chunk *chunk) {
    Scanner scanner;
    scanner_init(&scanner, source);

    Parser parser;
    parser.had_error = false;
    parser.panic_mode = false;
    parser.compiling_chunk = chunk;

    advance(&scanner, &parser);
    consume(&scanner, &parser, TOKEN_EOF, "Expect end of expression.");

    emit_return(&parser);

    return !parser.had_error;
}
