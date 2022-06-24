#include <stdio.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
    VM vm; vm_init(&vm);
    Chunk chunk; chunk_init(&chunk);

    chunk_push_constant(&chunk, 1.2, 1);
    chunk_push(&chunk, OP_NEGATE, 1);
    chunk_push(&chunk, OP_RETURN, 2);

    if (argc > 1 && strcmp(argv[1], "-d") == 0)
        disassemble_chunk(&chunk, "main");

    vm_interpret(&vm, &chunk);
    vm_free(&vm);
    chunk_free(&chunk);

    return 0;
}
