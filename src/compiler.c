#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "debug.h"
#include "scanner.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} Precedence;

typedef struct {
    Token prev;
    Token curr;
    bool had_error;
    bool panic_mode;
    Chunk *compiling_chunk;
    Scanner scanner;
} Parser;

typedef void (*ParseFn)(Parser *parser);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static void parser_init(Parser *parser, Chunk *chunk) {
    parser->had_error = false;
    parser->panic_mode = false;
    parser->compiling_chunk = chunk;
}

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

static void advance(Parser *parser) {
    parser->prev = parser->curr;

    for (;;) {
        parser->curr = scanner_scan_token(&parser->scanner);
        if (parser->curr.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->curr.start);
    }
}

static void consume(Parser *parser, TokenType type, const char *message) {
    if (parser->curr.type == type) {
        advance(parser);
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

static void expression(Parser *parser);
static ParseRule *get_rule(TokenType type);
static void parse_precedence(Parser *parser, Precedence precedence);

static void binary(Parser *parser) {
    TokenType operator_type = parser->prev.type;
    ParseRule *rule = get_rule(operator_type);
    parse_precedence(parser, (Precedence)(rule->precedence + 1));

    switch(operator_type) {
        case TOKEN_PLUS:
            emit_byte(parser, OP_ADD); break;
        case TOKEN_MINUS:
            emit_byte(parser, OP_SUBTRACT); break;
        case TOKEN_STAR:
            emit_byte(parser, OP_MULTIPLY); break;
        case TOKEN_SLASH:
            emit_byte(parser, OP_DIVIDE); break;

        default:
            return;
    }
}

static void grouping(Parser *parser) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(Parser *parser) {
    chunk_push_constant(
        parser->compiling_chunk,
        strtod(parser->prev.start, NULL), parser->prev.line);
}

static void unary(Parser *parser) {
    TokenType operator = parser->prev.type;
    parse_precedence(parser, PREC_UNARY);

    switch (operator) {
        case TOKEN_MINUS:
            emit_byte(parser, OP_NEGATE);
            break;
        default:
            return;
    }
}

ParseRule RULES[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FN]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LET]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule *get_rule(TokenType type) {
    return &RULES[type];
}

static void parse_precedence(Parser *parser, Precedence precedence) {
    advance(parser);
    ParseFn prefix_rule = get_rule(parser->prev.type)->prefix;

    if (prefix_rule == NULL) {
        error_at(parser, &parser->prev, "Expect expression.");
        return;
    }

    prefix_rule(parser);

    while (precedence <= get_rule(parser->curr.type)->precedence) {
        advance(parser);
        get_rule(parser->prev.type)->infix(parser);
    }
}

static void expression(Parser *parser) {
    parse_precedence(parser, PREC_ASSIGNMENT);
}

bool compile(const char *source, Chunk *chunk) {
    Parser parser;
    parser_init(&parser, chunk);
    scanner_init(&parser.scanner, source);

    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expect end of expression.");

    emit_return(&parser);

#ifdef DEBUG
    disassemble_chunk(parser.compiling_chunk, "chunk");
#endif

    return !parser.had_error;
}
