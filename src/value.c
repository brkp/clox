#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void value_array_init(ValueArray *array) {
    array->len = 0;
    array->cap = 0;
    array->values = NULL;
}

void value_array_free(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->cap);
    value_array_init(array);
}

void value_array_push(ValueArray *array, Value value) {
    if (array->len + 1 >= array->cap) {
        int old_cap = array->cap;
        array->cap = GROW_CAPACITY(old_cap);
        array->values = GROW_ARRAY(Value, array->values, old_cap, array->cap);
    }

    array->values[array->len++] = value;
}

void value_print(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:    printf("nil"); break;
        case VAL_OBJ:    object_print(value); break;
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    }
}

void object_print(Value value) {
    switch(OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type)
        return false;

    switch(a.type) {
        case VAL_NIL:    return true;
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            ObjString *a_string = AS_STRING(a);
            ObjString *b_string = AS_STRING(b);
            return a_string->len == b_string->len &&
                memcmp(a_string->data, b_string->data, a_string->len) == 0;
        }

        default:
            return false;
    }
}
