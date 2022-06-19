#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[]) {
    Chunk chunk;
    chunk_init(&chunk);

    for (int i = 0; i < 5; i++) {
        chunk_write(&chunk, OP_RETURN);
    }

    disassemble_chunk(&chunk, "main");
    chunk_free(&chunk);
    return 0;
}
