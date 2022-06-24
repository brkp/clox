#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
    VM vm;
    Chunk chunk;

    vm_init(&vm);
    chunk_init(&chunk);

    for (int i = 0; i < 5; i++) {
        chunk_push(&chunk, OP_CONSTANT, i);
        chunk_push(&chunk, chunk_add_constant(&chunk, i / 3.0), i);
    }
    chunk_push(&chunk, OP_RETURN, 5);

    disassemble_chunk(&chunk, "main");
    vm_interpret(&vm, &chunk);
    vm_free(&vm);
    chunk_free(&chunk);

    return 0;
}
