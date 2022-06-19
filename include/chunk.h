#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_RETURN,
} OpCode;

typedef struct {
    int len;
    int cap;
    u8 *code;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_write(Chunk *chunk, u8 byte);

int chunk_add_constant(Chunk *chunk, Value value);

#endif
