#include "memory.h"
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
