#include <stdio.h>

#include "vm.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"

static void reset_stack(VM *vm) {
    vm->sp = vm->stack;
}

void vm_init(VM *vm) {
    reset_stack(vm);
}

void vm_free(VM *vm) {}

void vm_stack_push(VM *vm, Value value) {
    *(vm->sp++) = value;
}

Value vm_stack_pop(VM *vm) {
    return *(--vm->sp);
}

static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
    (vm->chunk->constants.values[READ_BYTE() << 8 | READ_BYTE()])

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        if (vm->sp != vm->stack) {
            printf("\t");
            for (Value *slot = vm->stack; slot < vm->sp; slot++) {
                printf("[");
                value_print(*slot);
                printf("]");
            }
            printf("\n");
        }

        disassemble_opcode(vm->chunk, (int)(vm->ip - vm->chunk->code));
#endif
        uint8_t instruction;
        switch(instruction = READ_BYTE()) {
            case OP_CONSTANT:
                vm_stack_push(vm, READ_CONSTANT());
                break;
            case OP_CONSTANT_LONG:
                vm_stack_push(vm, READ_CONSTANT_LONG());
                break;
            case OP_NEGATE:
                vm_stack_push(vm, -vm_stack_pop(vm));
                break;
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult vm_interpret(VM *vm, Chunk *chunk) {
    vm->chunk = chunk;
    vm->ip = chunk->code;

    return run(vm);
}
