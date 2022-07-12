#include <stdio.h>
#include <stdarg.h>

#include "value.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"

static bool is_falsy(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void reset_stack(VM *vm) {
    vm->sp = vm->stack;
}

static void runtime_error(VM *vm, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    int line = vm->chunk->line[vm->ip - vm->chunk->code - 1];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack(vm);
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

Value vm_stack_peek(VM *vm, int distance) {
    return vm->sp[-1 - distance];
}

static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG()                                                   \
    (vm->chunk->constants.values[READ_BYTE() << 8 | READ_BYTE()])
#define BINARY_OP(value_type, op)                                              \
    do {                                                                       \
        if (!IS_NUMBER(vm_stack_peek(vm, 0)) ||                                \
            !IS_NUMBER(vm_stack_peek(vm, 1))) {                                \
            runtime_error(vm, "Operands must be numbers.");                    \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(vm_stack_pop(vm));                                \
        double a = AS_NUMBER(vm_stack_pop(vm));                                \
        vm_stack_push(vm, value_type(a op b));                                 \
    } while (false);

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

            case OP_NOT:
                vm_stack_push(vm, BOOL_VAL(is_falsy(vm_stack_pop(vm))));
                break;
            case OP_NIL:   vm_stack_push(vm, NIL_VAL); break;
            case OP_TRUE:  vm_stack_push(vm, BOOL_VAL(true)); break;
            case OP_FALSE: vm_stack_push(vm, BOOL_VAL(false)); break;

            case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;

            case OP_NEGATE: {
                if (!IS_NUMBER(vm_stack_peek(vm, 0))) {
                    runtime_error(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_stack_push(vm, NUMBER_VAL(-AS_NUMBER(vm_stack_pop(vm))));
                break;
            }
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef BINARY_OP
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult vm_interpret(VM *vm, const char *source) {
    Chunk chunk; chunk_init(&chunk);

    if (!compile(source, &chunk)) {
        chunk_free(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);
    chunk_free(&chunk);

    return result;
}
