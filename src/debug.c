#include <stdio.h>
#include "debug.h"

static int simple_opcode(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

void disassemble_chunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->len;) {
        offset = disassemble_opcode(chunk, offset);
    }
}

int disassemble_opcode(Chunk *chunk, int offset) {
    printf("%04d ", offset);

    u8 opcode = chunk->code[offset];
    switch (opcode) {
        case OP_RETURN:
            return simple_opcode("OP_RETURN", offset);
        default:
            printf("unknown opcode: %" PRIu8 "\n", opcode);
            return offset + 1;
    }
}
