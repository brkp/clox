#ifndef clox_compiler_h
#define clox_compiler_h

#include "common.h"
#include "chunk.h"
#include "vm.h"

bool compile(const char *source, VM *vm, Chunk *chunk);

#endif
