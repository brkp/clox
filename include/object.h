#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "vm.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_STRING(value)    is_obj_type(value, OBJ_STRING)

#define AS_STRING(value)    ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString *)AS_OBJ(value))->data)

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
};

struct ObjString {
    Obj obj;
    int len;
    char *data;
    uint32_t hash;
};

ObjString *take_string(VM *vm, char *data, int len);
ObjString *copy_string(VM *vm, const char *data, int len);

static inline bool is_obj_type(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
