#include "vm.h"
#include "chunk.h"

void vm_init(VM *vm) {}
void vm_free(VM *vm) {}

static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)

    for (;;) {
        uint8_t instruction;
        switch(instruction = READ_BYTE()) {
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef READ_BYTE
}

InterpretResult vm_interpret(VM *vm, Chunk *chunk) {
    vm->chunk = chunk;
    vm->ip = chunk->code;

    return run(vm);
}
