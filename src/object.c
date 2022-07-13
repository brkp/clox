#include <string.h>

#include "object.h"
#include "memory.h"

#define ALLOCATE_OBJ(vm, type, object_type)                                        \
    (type *)allocate_obj(vm, sizeof(type), object_type)

static Obj *allocate_obj(VM *vm, size_t size, ObjType object_type) {
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = object_type;
    object->next = vm->objects;
    vm->objects = object;
    return object;
}

static ObjString *allocate_string(VM *vm, char *data, int len) {
    ObjString *string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->len = len;
    string->data = data;
    return string;
}

ObjString *take_string(VM *vm, char *data, int len) {
    return allocate_string(vm, data, len);
}

ObjString *copy_string(VM *vm, const char *data, int len) {
    char *heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, data, len);
    heap_chars[len] = '\0';
    return allocate_string(vm, heap_chars, len);
}
