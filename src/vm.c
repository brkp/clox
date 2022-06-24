#include <stdio.h>

#include "vm.h"
#include "chunk.h"
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
        disassemble_opcode(vm->chunk, (int)(vm->ip - vm->chunk->code));
#endif
        uint8_t instruction;
        switch(instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                value_print(constant); printf("\n");
                break;
            }
            case OP_CONSTANT_LONG: {
                Value constant = READ_CONSTANT_LONG();
                value_print(constant); printf("\n");
                break;
            }
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
