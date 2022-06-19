#include "chunk.h"
#include "memory.h"

void chunk_init(Chunk *chunk) {
    chunk->len = 0;
    chunk->cap = 0;
    chunk->code = NULL;
    value_array_init(&chunk->constants);
}

void chunk_free(Chunk *chunk) {
    FREE_ARRAY(u8, chunk->code, chunk->cap);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_write(Chunk *chunk, u8 byte) {
    if (chunk->len + 1 >= chunk->cap) {
        int old_cap = chunk->cap;
        chunk->cap  = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(u8, chunk->code, old_cap, chunk->cap);
    }

    chunk->code[chunk->len++] = byte;
}
