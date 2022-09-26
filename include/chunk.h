#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NOT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_EQUAL,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    OP_LESS,
    OP_GREATER,
    OP_ADD,
    OP_SET_LOCAL,
    OP_SET_LOCAL_LONG,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,
} OpCode;

typedef struct {
    int len;
    int cap;
    int *line;
    uint8_t *code;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_push(Chunk *chunk, uint8_t byte, int line);

uint16_t chunk_push_constant(Chunk *chunk, Value value, int line);

#endif
