#ifndef clox_compiler_h
#define clox_compiler_h

#include "common.h"
#include "chunk.h"
#include "scanner.h"
#include "vm.h"

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
    Token name;
    int depth;
} Local;

typedef struct {
    Local locals[UINT8_MAX + 1];
    int local_count;
    int scope_depth;
    Chunk *compiling_chunk;
} Compiler;

typedef struct {
    Token prev;
    Token curr;
    bool had_error;
    bool panic_mode;
} Parser;

typedef struct {
    VM *vm;

    Parser parser;
    Scanner scanner;
    Compiler compiler;
} State;

typedef void (*ParseFn)(State *state, bool can_assign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;


bool compile(const char *source, VM *vm, Chunk *chunk);

#endif
