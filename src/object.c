#include <string.h>

#include "object.h"
#include "memory.h"

#define ALLOCATE_OBJ(type, object_type)                                        \
    (type *)allocate_obj(sizeof(type), object_type)

static Obj *allocate_obj(size_t size, ObjType object_type) {
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = object_type;
    return object;
}

static ObjString *allocate_string(char *data, int len) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->len = len;
    string->data = data;
    return string;
}

ObjString *take_string(char *data, int len) {
    return allocate_string(data, len);
}

ObjString *copy_string(const char *data, int len) {
    char *heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, data, len);
    heap_chars[len] = '\0';
    return allocate_string(heap_chars, len);
}
