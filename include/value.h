#ifndef clox_value_h
#define clox_value_h

#include "common.h"

#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value)    ((value).type == VAL_OBJ)

#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value)    ((value).as.obj)

#define BOOL_VAL(value)   ((Value){VAL_BOOL,   { .boolean = value }})
#define NIL_VAL           ((Value){VAL_NIL,    { .number = 0 }})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, { .number = value }})
#define OBJ_VAL(object)   ((Value){VAL_OBJ,    { .obj = (Obj *)object }})

typedef struct Obj Obj;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj *obj;
    } as;
} Value;

typedef struct {
    int len;
    int cap;
    Value *values;
} ValueArray;

void value_array_init(ValueArray *array);
void value_array_free(ValueArray *array);
void value_array_push(ValueArray *array, Value value);

void value_print(Value value);
bool values_equal(Value a, Value b);

#endif
