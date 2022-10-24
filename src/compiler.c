#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "value.h"
#include "scanner.h"

static void compiler_init(Compiler *compiler, Chunk *chunk) {
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->compiling_chunk = chunk;
}

static void parser_init(Parser *parser) {
    parser->had_error = false;
    parser->panic_mode = false;
}

static void state_init(State *state, const char *source, VM *vm, Chunk *chunk) {
    state->vm = vm;
    parser_init(&state->parser);
    scanner_init(&state->scanner, source);
    compiler_init(&state->compiler, chunk);
}

static void error_at(State *state, Token *token, const char *message) {
    if (state->parser.panic_mode)
        return;

    state->parser.had_error = true;
    state->parser.panic_mode = true;

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

static void error_at_current(State *state, const char *message) {
    error_at(state, &state->parser.curr, message);
}

static void advance(State *state) {
    state->parser.prev = state->parser.curr;

    for (;;) {
        state->parser.curr = scanner_scan_token(&state->scanner);
        if (state->parser.curr.type != TOKEN_ERROR) break;

        error_at_current(state, state->parser.curr.start);
    }
}

static void consume(State *state, TokenType type, const char *message) {
    if (state->parser.curr.type == type) {
        advance(state);
        return;
    }

    error_at_current(state, message);
}

static bool check(State *state, TokenType type) {
    return state->parser.curr.type == type;
}

static bool match(State *state, TokenType type) {
    if (check(state, type)) {
        advance(state);
        return true;
    }

    return false;
}

static void emit_byte(State *state, uint8_t byte) {
    chunk_push(state->compiler.compiling_chunk, byte, state->parser.prev.line);
}

static int emit_jump(State *state, uint8_t instruction) {
    emit_byte(state, instruction);
    emit_byte(state, 0xff);
    emit_byte(state, 0xff);

    return state->compiler.compiling_chunk->len - 2;
}

static void emit_loop(State *state, int loop_start) {
    emit_byte(state, OP_LOOP);

    int offset = state->compiler.compiling_chunk->len - loop_start + 2;
    if (offset > UINT16_MAX)
        error_at_current(state, "Loop body too large.");

    emit_byte(state, (offset >> 8) & 0xff);
    emit_byte(state, (offset >> 0) & 0xff);
}

static void patch_jump(State *state, int offset) {
    int jump = state->compiler.compiling_chunk->len - offset - 2;
    if (jump > UINT16_MAX)
        error_at_current(state, "Too much code to jump over.");

    state->compiler.compiling_chunk->code[offset + 0] = (jump >> 8) & 0xff;
    state->compiler.compiling_chunk->code[offset + 1] = (jump >> 0) & 0xff;
}

static void emit_return(State *state) {
    emit_byte(state, OP_RETURN);
}

static void expression(State *state);
static void statement(State *state);
static void declaration(State *state);
static ParseRule *get_rule(TokenType type);
static void parse_precedence(State *state, Precedence precedence);

static uint16_t identifier_constant(State *state, Token *name) {
    value_array_push(
        &state->compiler.compiling_chunk->constants,
        OBJ_VAL(copy_string(state->vm, name->start, name->length)));

    return (uint16_t)(state->compiler.compiling_chunk->constants.len - 1);
}

static bool identifiers_equal(Token *a, Token *b) {
    if (a->length != b->length)
        return false;

    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(State *state, Token *name) {
    for (int i = state->compiler.local_count - 1; i >= 0; i--) {
        Local *local = &state->compiler.locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1)
                error_at_current(state, "Can't read local variable in its own initializer.");

            return i;
        }
    }

    return -1;
}

static void add_local(State *state, Token name) {
    if (state->compiler.local_count == UINT8_MAX + 1) {
        error_at_current(state, "Too many local variables in function.");
        return;
    }

    Local *local = &state->compiler.locals[state->compiler.local_count++];
    local->name = name;
    local->depth = -1;
}

static void declare_variable(State *state) {
    if (state->compiler.scope_depth == 0)
        return;

    Token *name = &state->parser.prev;
    for (int i = state->compiler.local_count - 1; i >= 0; i--) {
        Local *local = &state->compiler.locals[i];

        if (local->depth != -1 && local->depth < state->compiler.scope_depth) {
            break;
        }

        if (identifiers_equal(name, &local->name)) {
            error_at_current(state, "Already a variable with this name in this scope.");
        }
    }

    add_local(state, *name);
}

static void and_(State *state, bool can_assign) {
    (void)can_assign;

    int end_jump = emit_jump(state, OP_JUMP_IF_FALSE);

    emit_byte(state, OP_POP);
    parse_precedence(state, PREC_AND);

    patch_jump(state, end_jump);
}

static void or_(State *state, bool can_assign) {
    (void)can_assign;

    int else_jump = emit_jump(state, OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(state, OP_JUMP);

    patch_jump(state, else_jump);
    emit_byte(state, OP_POP);

    parse_precedence(state, PREC_OR);
    patch_jump(state, end_jump);
}

static void binary(State *state, bool can_assign) {
    (void)can_assign;

    TokenType operator_type = state->parser.prev.type;
    ParseRule *rule = get_rule(operator_type);
    parse_precedence(state, (Precedence)(rule->precedence + 1));

    switch(operator_type) {
        case TOKEN_EQUAL_EQUAL: emit_byte(state, OP_EQUAL); break;
        case TOKEN_GREATER:     emit_byte(state, OP_GREATER); break;
        case TOKEN_LESS:        emit_byte(state, OP_LESS); break;
        case TOKEN_BANG_EQUAL:
            emit_byte(state, OP_EQUAL);
            emit_byte(state, OP_NOT);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_byte(state, OP_LESS);
            emit_byte(state, OP_NOT);
            break;
        case TOKEN_LESS_EQUAL:
            emit_byte(state, OP_GREATER);
            emit_byte(state, OP_NOT);
            break;

        case TOKEN_PLUS:  emit_byte(state, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(state, OP_SUBTRACT); break;
        case TOKEN_STAR:  emit_byte(state, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(state, OP_DIVIDE); break;

        default: // unreachable
            return;
    }
}

static void literal(State *state, bool can_assign) {
    (void)can_assign;

    switch (state->parser.prev.type) {
        case   TOKEN_NIL: emit_byte(state,   OP_NIL); break;
        case  TOKEN_TRUE: emit_byte(state,  OP_TRUE); break;
        case TOKEN_FALSE: emit_byte(state, OP_FALSE); break;

        default: // unreachable
            return;
    }
}

static void grouping(State *state, bool can_assign) {
    (void)can_assign;

    expression(state);
    consume(state, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(State *state, bool can_assign) {
    (void)can_assign;

    chunk_push_constant(
        state->compiler.compiling_chunk,
        NUMBER_VAL(strtod(state->parser.prev.start, NULL)), state->parser.prev.line);
}

static void string(State *state, bool can_assign) {
    (void)can_assign;

    chunk_push_constant(
        state->compiler.compiling_chunk,
        OBJ_VAL(copy_string(
                state->vm,
                state->parser.prev.start + 1,
                state->parser.prev.length - 2)),
        state->parser.prev.line);
}

static void named_variable(State *state, bool can_assign) {
    uint16_t offset; bool is_long;
    uint8_t instruction, get_instruction, set_instruction;

    Token *name = &state->parser.prev;
    int local_offset = resolve_local(state, name);

    if (local_offset != -1) {
        offset = local_offset;
        is_long = offset > 0xff;
        set_instruction = is_long ? OP_SET_LOCAL_LONG : OP_SET_LOCAL;
        get_instruction = is_long ? OP_GET_LOCAL_LONG : OP_GET_LOCAL;
    }
    else {
        offset = identifier_constant(state, name);
        is_long = offset > 0xff;
        set_instruction = is_long ? OP_SET_GLOBAL_LONG : OP_SET_GLOBAL;
        get_instruction = is_long ? OP_GET_GLOBAL_LONG : OP_GET_GLOBAL;
    }

    if (!can_assign || !match(state, TOKEN_EQUAL))
        instruction = get_instruction;
    else {
        expression(state);
        instruction = set_instruction;
    }

    emit_byte(state, instruction);
    if (is_long)
        emit_byte(state, offset >> 0x8);
    emit_byte(state, offset & 0xff);
}

static void variable(State *state, bool can_assign) {
    named_variable(state, can_assign);
}

static void unary(State *state, bool can_assign) {
    (void)can_assign;

    TokenType operator = state->parser.prev.type;
    parse_precedence(state, PREC_UNARY);

    switch (operator) {
        case  TOKEN_BANG: emit_byte(state, OP_NOT); break;
        case TOKEN_MINUS: emit_byte(state, OP_NEGATE); break;

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
    [TOKEN_IDENTIFIER]    = {variable,     NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FN]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
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

static void parse_precedence(State *state, Precedence precedence) {
    advance(state);
    ParseFn prefix_rule = get_rule(state->parser.prev.type)->prefix;

    if (prefix_rule == NULL) {
        error_at(state, &state->parser.prev, "Expect expression.");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(state, can_assign);

    while (precedence <= get_rule(state->parser.curr.type)->precedence) {
        advance(state);
        get_rule(state->parser.prev.type)->infix(state, can_assign);
    }

    if (can_assign && match(state, TOKEN_EQUAL))
        error_at_current(state, "Invalid assignment target.");
}

static void expression(State *state) {
    parse_precedence(state, PREC_ASSIGNMENT);
}

static uint16_t parse_variable(State *state, const char *message) {
    consume(state, TOKEN_IDENTIFIER, message);

    declare_variable(state);
    if (state->compiler.scope_depth > 0) return 0;

    return identifier_constant(state, &state->parser.prev);
}

static void mark_initialized(State *state) {
    if (state->compiler.scope_depth == 0)
        return;

    state->compiler.locals[state->compiler.local_count - 1].depth =
        state->compiler.scope_depth;
}

static void define_variable(State *state, uint16_t global) {
    if (state->compiler.scope_depth > 0) {
        mark_initialized(state);
        return;
    }

    if (global > 0xff) {
        emit_byte(state, OP_DEFINE_GLOBAL_LONG);
        emit_byte(state, global >> 0x8);
        emit_byte(state, global & 0xff);
    }
    else {
        emit_byte(state, OP_DEFINE_GLOBAL);
        emit_byte(state, global & 0xff);
    }
}

static void var_declaration(State *state) {
    uint16_t global = parse_variable(state, "Expect variable name.");

    if (match(state, TOKEN_EQUAL))
        expression(state);
    else
        emit_byte(state, OP_NIL);

    consume(state, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    define_variable(state, global);
}

static void expression_statement(State *state) {
    expression(state);
    consume(state, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(state, OP_POP);
}

static void print_statement(State *state) {
    expression(state);
    consume(state, TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(state, OP_PRINT);
}

static void if_statement(State *state) {
    consume(state, TOKEN_LEFT_PAREN, "Expect '(' after if.");
    expression(state);
    consume(state, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int then_jump = emit_jump(state, OP_JUMP_IF_FALSE);
    emit_byte(state, OP_POP);
    statement(state);
    int else_jump = emit_jump(state, OP_JUMP);
    patch_jump(state, then_jump);
    emit_byte(state, OP_POP);

    if (match(state, TOKEN_ELSE))
        statement(state);
    patch_jump(state, else_jump);
}

static void while_statement(State *state) {
    int loop_start = state->compiler.compiling_chunk->len;

    consume(state, TOKEN_LEFT_PAREN, "Expect '(' after while.");
    expression(state);
    consume(state, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = emit_jump(state, OP_JUMP_IF_FALSE);
    emit_byte(state, OP_POP);
    statement(state);
    emit_loop(state, loop_start);

    patch_jump(state, exit_jump);
    emit_byte(state, OP_POP);
}

static void block(State *state) {
    while (!check(state, TOKEN_RIGHT_BRACE) && !check(state, TOKEN_EOF))
        declaration(state);

    consume(state, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void synchronize(State *state) {
    state->parser.panic_mode = false;

    while (state->parser.curr.type != TOKEN_EOF) {
        if (state->parser.prev.type == TOKEN_SEMICOLON)
            return;

        switch (state->parser.prev.type) {
            case TOKEN_CLASS:
            case TOKEN_FN:
            case TOKEN_LET:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                ;
        }

        advance(state);
    }
}

static void declaration(State *state) {
    if (match(state, TOKEN_LET)) {
        var_declaration(state);
    }
    else {
        statement(state);
    }

    if (state->parser.panic_mode)
        synchronize(state);
}

static void begin_scope(State *state) {
    state->compiler.scope_depth++;
}

static void end_scope(State *state) {
    int depth = --state->compiler.scope_depth;

    while (state->compiler.local_count > 0 &&
           state->compiler.locals[state->compiler.local_count - 1].depth > depth)
    {
        emit_byte(state, OP_POP);
        state->compiler.local_count--;
    }
}

static void statement(State *state) {
    if (match(state, TOKEN_PRINT)) {
        print_statement(state);
    }
    else if (match(state, TOKEN_IF)) {
        if_statement(state);
    }
    else if (match(state, TOKEN_WHILE)) {
        while_statement(state);
    }
    else if (match(state, TOKEN_LEFT_BRACE)) {
        begin_scope(state);
        block(state);
        end_scope(state);
    }
    else {
        expression_statement(state);
    }
}

bool compile(const char *source, VM *vm, Chunk *chunk) {
    State state;
    state_init(&state, source, vm, chunk);

    advance(&state);
    while (!match(&state, TOKEN_EOF)) {
        declaration(&state);
    }

    emit_return(&state);

#ifdef DEBUG
    disassemble_chunk(state.compiler.compiling_chunk, "chunk");
    printf("\n");
#endif

    return !state.parser.had_error;
}
