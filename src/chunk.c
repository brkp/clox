#include "chunk.h"
#include "memory.h"

void chunk_init(Chunk *chunk) {
    chunk->len = 0;
    chunk->cap = 0;
    chunk->code = NULL;
    value_array_init(&chunk->constants);
}

void chunk_free(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_push(Chunk *chunk, uint8_t byte) {
    if (chunk->len + 1 >= chunk->cap) {
        int old_cap = chunk->cap;
        chunk->cap  = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->cap);
    }

    chunk->code[chunk->len++] = byte;
}

int chunk_add_constant(Chunk *chunk, Value value) {
    value_array_push(&chunk->constants, value);
    return chunk->constants.len - 1;
}
