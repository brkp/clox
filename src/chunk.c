#include "chunk.h"
#include "memory.h"

void chunk_init(Chunk *chunk) {
    chunk->len = 0;
    chunk->cap = 0;

    chunk->code = NULL;
    chunk->line = NULL;
    value_array_init(&chunk->constants);
}

void chunk_free(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
    FREE_ARRAY(int, chunk->line, chunk->cap);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_push(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->len + 1 >= chunk->cap) {
        int old_cap = chunk->cap;
        chunk->cap  = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->cap);
        chunk->line = GROW_ARRAY(int, chunk->line, old_cap, chunk->cap);
    }

    chunk->code[chunk->len] = byte;
    chunk->line[chunk->len] = line;
    chunk->len++;
}

void chunk_push_constant(Chunk *chunk, Value value, int line) {
    value_array_push(&chunk->constants, value);
    int offset = chunk->constants.len - 1;

    if (offset > 0xff) {
        chunk_push(chunk, OP_CONSTANT_LONG, line);
        chunk_push(chunk, (offset >> 8) & 0xff, line);
        chunk_push(chunk, (offset >> 0) & 0xff, line);
    }
    else {
        chunk_push(chunk, OP_CONSTANT, line);
        chunk_push(chunk, offset, line);
    }
}
