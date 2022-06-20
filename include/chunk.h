#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
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

int chunk_add_constant(Chunk *chunk, Value value);

#endif
