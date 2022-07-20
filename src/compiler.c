#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "value.h"
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
    VM *vm;
    Chunk *compiling_chunk;
    Scanner scanner;
} Parser;

typedef void (*ParseFn)(Parser *parser);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static void parser_init(Parser *parser, VM *vm, Chunk *chunk) {
    parser->had_error = false;
    parser->panic_mode = false;
    parser->vm = vm;
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

static bool check(Parser *parser, TokenType type) {
    return parser->curr.type == type;
}

static bool match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }

    return false;
}

static void emit_byte(Parser *parser, uint8_t byte) {
    chunk_push(parser->compiling_chunk, byte, parser->prev.line);
}

static void emit_return(Parser *parser) {
    emit_byte(parser, OP_RETURN);
}

static void expression(Parser *parser);
static void statement(Parser *parser);
static void declaration(Parser *parser);
static ParseRule *get_rule(TokenType type);
static void parse_precedence(Parser *parser, Precedence precedence);

static void binary(Parser *parser) {
    TokenType operator_type = parser->prev.type;
    ParseRule *rule = get_rule(operator_type);
    parse_precedence(parser, (Precedence)(rule->precedence + 1));

    switch(operator_type) {
        case TOKEN_EQUAL_EQUAL: emit_byte(parser, OP_EQUAL); break;
        case TOKEN_GREATER:     emit_byte(parser, OP_GREATER); break;
        case TOKEN_LESS:        emit_byte(parser, OP_LESS); break;
        case TOKEN_BANG_EQUAL:
            emit_byte(parser, OP_EQUAL);
            emit_byte(parser, OP_NOT);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_byte(parser, OP_LESS);
            emit_byte(parser, OP_NOT);
            break;
        case TOKEN_LESS_EQUAL:
            emit_byte(parser, OP_GREATER);
            emit_byte(parser, OP_NOT);
            break;

        case TOKEN_PLUS:  emit_byte(parser, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(parser, OP_SUBTRACT); break;
        case TOKEN_STAR:  emit_byte(parser, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(parser, OP_DIVIDE); break;

        default: // unreachable
            return;
    }
}

static void literal(Parser *parser) {
    switch (parser->prev.type) {
        case TOKEN_NIL: emit_byte(parser, OP_NIL); break;
        case TOKEN_TRUE: emit_byte(parser, OP_TRUE); break;
        case TOKEN_FALSE: emit_byte(parser, OP_FALSE); break;

        default: // unreachable
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
        NUMBER_VAL(strtod(parser->prev.start, NULL)), parser->prev.line);
}

static void string(Parser *parser) {
    chunk_push_constant(
        parser->compiling_chunk,
        OBJ_VAL(copy_string(
                parser->vm,
                parser->prev.start + 1,
                parser->prev.length - 2)),
        parser->prev.line);
}

static void unary(Parser *parser) {
    TokenType operator = parser->prev.type;
    parse_precedence(parser, PREC_UNARY);

    switch (operator) {
        case TOKEN_BANG:  emit_byte(parser, OP_NOT); break;
        case TOKEN_MINUS: emit_byte(parser, OP_NEGATE); break;

        default: // unreachable
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
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {binary,   NULL,   PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FN]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
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

static void expression_statement(Parser *parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(parser, OP_POP);
}

static void print_statement(Parser *parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(parser, OP_PRINT);
}

static void declaration(Parser *parser) {
    statement(parser);
}

static void statement(Parser *parser) {
    if (match(parser, TOKEN_PRINT)) {
        print_statement(parser);
    }
    else {
        expression_statement(parser);
    }
}

bool compile(const char *source, VM *vm, Chunk *chunk) {
    Parser parser;
    parser_init(&parser, vm, chunk);
    scanner_init(&parser.scanner, source);

    advance(&parser);
    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    emit_return(&parser);

#ifdef DEBUG
    disassemble_chunk(parser.compiling_chunk, "chunk");
#endif

    return !parser.had_error;
}
