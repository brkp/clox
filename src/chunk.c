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

void write_chunk(Chunk *chunk, u8 byte) {}
