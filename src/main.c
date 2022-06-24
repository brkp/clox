#include <stdio.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
    VM vm; vm_init(&vm);
    Chunk chunk; chunk_init(&chunk);

    // -(3 * (1 + 2))
    chunk_push_constant(&chunk, 3, 3);
    chunk_push_constant(&chunk, 1, 1);
    chunk_push_constant(&chunk, 2, 1);
    chunk_push(&chunk, OP_ADD, 1);
    chunk_push(&chunk, OP_MULTIPLY, 1);
    chunk_push(&chunk, OP_NEGATE, 1);

    chunk_push(&chunk, OP_RETURN, 2);

    if (argc > 1 && strcmp(argv[1], "-d") == 0)
        disassemble_chunk(&chunk, "main");

    vm_interpret(&vm, &chunk);
    vm_free(&vm);
    chunk_free(&chunk);

    return 0;
}
