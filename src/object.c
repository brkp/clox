#include <string.h>

#include "memory.h"
#include "object.h"
#include "utils.h"
#include "value.h"

#define ALLOCATE_OBJ(vm, type, object_type)                                    \
    (type *)allocate_obj(vm, sizeof(type), object_type)

static Obj *allocate_obj(VM *vm, size_t size, ObjType object_type) {
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = object_type;
    object->next = vm->objects;
    vm->objects = object;
    return object;
}

static ObjString *allocate_string(VM *vm, char *data, int len, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->len = len;
    string->data = data;
    string->hash = hash;
    table_set(&vm->strings, string, NIL_VAL);
    return string;
}

ObjString *take_string(VM *vm, char *data, int len) {
    uint32_t hash = hash_string(data, len);
    ObjString *interned = table_find_string(&vm->strings, data, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, data, len + 1);
        return interned;
    }

    return allocate_string(vm, data, len, hash);
}

ObjString *copy_string(VM *vm, const char *data, int len) {
    uint32_t hash = hash_string(data, len);
    ObjString *interned = table_find_string(&vm->strings, data, len, hash);
    if (interned != NULL) return interned;

    char *heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, data, len);
    heap_chars[len] = '\0';
    return allocate_string(vm, heap_chars, len, hash);
}
