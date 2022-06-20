#include <stdio.h>

#include "debug.h"
#include "value.h"

static int simple_opcode(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constant_opcode(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4" PRIu8 " '", name, constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 2;
}

void disassemble_chunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->len;) {
        offset = disassemble_opcode(chunk, offset);
    }
}

int disassemble_opcode(Chunk *chunk, int offset) {
    printf("%04d ", offset);

    uint8_t opcode = chunk->code[offset];
    switch (opcode) {
        case OP_RETURN:
            return simple_opcode("OP_RETURN", offset);
        case OP_CONSTANT:
            return constant_opcode("OP_CONSTANT", chunk, offset);
        default:
            printf("unknown opcode: %" PRIu8 "\n", opcode);
            return offset + 1;
    }
}
