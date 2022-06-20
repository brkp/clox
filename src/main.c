#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[]) {
    Chunk chunk;
    chunk_init(&chunk);

    for (int i = 0; i < 5; i++) {
        chunk_push(&chunk, OP_CONSTANT, i);
        chunk_push(&chunk, chunk_add_constant(&chunk, i / 3.0), i);
    }
    chunk_push(&chunk, OP_RETURN, 5);

    disassemble_chunk(&chunk, "main");
    chunk_free(&chunk);
    return 0;
}
