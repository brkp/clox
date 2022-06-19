#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *chunk) {
    chunk->len = 0;
    chunk->cap = 0;
    chunk->code = NULL;
}

void free_chunk(Chunk *chunk) {
    FREE_ARRAY(u8, chunk->code, chunk->cap);
}

void write_chunk(Chunk *chunk, u8 byte) {
    if (chunk->len + 1 >= chunk->cap) {
        int old_cap = chunk->cap;
        chunk->cap  = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(u8, chunk->code, old_cap, chunk->cap);
    }

    chunk->code[chunk->len++] = byte;
}
