#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define STACK_MAX 1024

typedef struct {
    Chunk *chunk;
    Value stack[STACK_MAX];
    uint8_t *ip;
    Value *sp;
    Obj *objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void vm_init(VM *vm);
void vm_free(VM *vm);

void vm_stack_push(VM *vm, Value value);
Value vm_stack_pop(VM *vm);

InterpretResult vm_interpret(VM *vm, const char *source);

#endif
