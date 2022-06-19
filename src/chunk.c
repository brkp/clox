#include "chunk.h"
#include "memory.h"

void chunk_init(Chunk *chunk) {
    chunk->len = 0;
    chunk->cap = 0;
    chunk->code = NULL;
}

void chunk_free(Chunk *chunk) {
    FREE_ARRAY(u8, chunk->code, chunk->cap);
}

void chunk_write(Chunk *chunk, u8 byte) {
    if (chunk->len + 1 >= chunk->cap) {
        int old_cap = chunk->cap;
        chunk->cap  = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(u8, chunk->code, old_cap, chunk->cap);
    }

    chunk->code[chunk->len++] = byte;
}
