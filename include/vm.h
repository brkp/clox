#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"

typedef struct {
    Chunk *chunk;
    uint8_t *ip;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void vm_init(VM *vm);
void vm_free(VM *vm);

InterpretResult vm_interpret(VM *vm, Chunk *chunk);

#endif
