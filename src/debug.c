#include <stdio.h>

#include "common.h"
#include "chunk.h"
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

static int constant_long_opcode(const char *name, Chunk *chunk, int offset) {
    uint16_t constant = chunk->code[offset + 1] << 8 | \
                        chunk->code[offset + 2];

    printf("%-16s %4" PRIu16 " '", name, constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 3;
}

void disassemble_chunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->len;) {
        offset = disassemble_opcode(chunk, offset);
    }
}

int disassemble_opcode(Chunk *chunk, int offset) {
    printf("%04d ", offset);

    if (offset > 0 && chunk->line[offset] == chunk->line[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->line[offset]);

    uint8_t opcode = chunk->code[offset];
    switch (opcode) {
        case OP_CONSTANT:
            return constant_opcode("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constant_long_opcode("OP_CONSTANT_LONG", chunk, offset);
        case OP_ADD:
            return simple_opcode("OP_ADD", offset);
        case OP_SUBTRACT:
            return simple_opcode("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_opcode("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_opcode("OP_DIVIDE", offset);
        case OP_NEGATE:
            return simple_opcode("OP_NEGATE", offset);
        case OP_RETURN:
            return simple_opcode("OP_RETURN", offset);
        default:
            printf("unknown opcode: %" PRIu8 "\n", opcode);
            return offset + 1;
    }
}
