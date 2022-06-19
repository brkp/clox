#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

typedef struct {
    int len;
    int cap;
    Value *values;
} ValueArray;

void value_array_init(ValueArray *array);
void value_array_free(ValueArray *array);
void value_array_push(ValueArray *array, Value value);

void value_print(Value value);

#endif
