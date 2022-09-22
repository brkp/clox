#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "memory.h"
#include "value.h"
#include "object.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"

static bool is_falsy(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(VM *vm) {
    ObjString *b = AS_STRING(vm_stack_pop(vm));
    ObjString *a = AS_STRING(vm_stack_pop(vm));

    int length = a->len + b->len;
    char *data = ALLOCATE(char, length + 1);
    memcpy(data, a->data, a->len);
    memcpy(data + a->len, b->data, b->len);
    data[length] = '\0';

    vm_stack_push(vm, OBJ_VAL(take_string(vm, data, length)));
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
    vm->objects = NULL;
    table_init(&vm->strings);
    table_init(&vm->globals);
}

void vm_free(VM *vm) {
    table_free(&vm->strings);
    table_free(&vm->globals);
    free_objects(vm->objects);
}

void vm_stack_push(VM *vm, Value value) {
    *(vm->sp++) = value;
}

Value vm_stack_pop(VM *vm) {
    return *(--vm->sp);
}

Value vm_stack_peek(VM *vm, int distance) {
    return vm->sp[-1 - distance];
}

static void global_define(VM *vm, ObjString *name) {
    table_set(&vm->globals, name, vm_stack_peek(vm, 0));
    vm_stack_pop(vm);
}

static int global_get(VM *vm, ObjString *name) {
    Value value;
    if (!table_get(&vm->globals, name, &value)) {
        runtime_error(vm, "Undefined variable '%s'.", name->data);
        return 1;
    }
    vm_stack_push(vm, value);

    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"
static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG()                                                   \
    (vm->chunk->constants.values[READ_BYTE() << 8 | READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())
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
            case OP_CONSTANT: vm_stack_push(vm, READ_CONSTANT()); break;
            case OP_CONSTANT_LONG: vm_stack_push(vm, READ_CONSTANT_LONG()); break;

            case OP_EQUAL: {
                Value b = vm_stack_pop(vm);
                Value a = vm_stack_pop(vm);
                vm_stack_push(vm, BOOL_VAL(values_equal(a, b)));
                break;
            }

            case OP_NOT:   vm_stack_push(vm, BOOL_VAL(is_falsy(vm_stack_pop(vm)))); break;
            case OP_NIL:   vm_stack_push(vm, NIL_VAL); break;
            case OP_TRUE:  vm_stack_push(vm, BOOL_VAL(true)); break;
            case OP_FALSE: vm_stack_push(vm, BOOL_VAL(false)); break;
            case OP_POP:   vm_stack_pop(vm); break;

            case OP_GET_GLOBAL: {
                if (global_get(vm, READ_STRING()) != 0)
                    return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_GET_GLOBAL_LONG: {
                if (global_get(vm, READ_STRING_LONG()) != 0)
                    return INTERPRET_RUNTIME_ERROR;
                break;
            }

            case OP_DEFINE_GLOBAL: global_define(vm, READ_STRING()); break;
            case OP_DEFINE_GLOBAL_LONG: global_define(vm, READ_STRING_LONG()); break;

            case OP_ADD: {
                if (IS_STRING(vm_stack_peek(vm, 0)) && IS_STRING(vm_stack_peek(vm, 1))) {
                    concatenate(vm);
                }
                else if (IS_NUMBER(vm_stack_peek(vm, 0)) && IS_NUMBER(vm_stack_peek(vm, 1))) {
                    double b = AS_NUMBER(vm_stack_pop(vm));
                    double a = AS_NUMBER(vm_stack_pop(vm));
                    vm_stack_push(vm, NUMBER_VAL(a + b));
                }
                else {
                    runtime_error(vm, "Operands must be two numbers or strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_GREATER:  BINARY_OP(BOOL_VAL,   >); break;
            case OP_LESS:     BINARY_OP(BOOL_VAL,   <); break;

            case OP_NEGATE: {
                if (!IS_NUMBER(vm_stack_peek(vm, 0))) {
                    runtime_error(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_stack_push(vm, NUMBER_VAL(-AS_NUMBER(vm_stack_pop(vm))));
                break;
            }
            case OP_PRINT: {
                value_print(vm_stack_pop(vm));
                printf("\n");
                break;
            }
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef BINARY_OP
#undef READ_STRING_LONG
#undef READ_STRING
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
}
#pragma GCC diagnostic pop

InterpretResult vm_interpret(VM *vm, const char *source) {
    Chunk chunk; chunk_init(&chunk);

    if (!compile(source, vm, &chunk)) {
        chunk_free(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);
    chunk_free(&chunk);

    return result;
}
