#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int len;
    int cap;
    Entry *entries;
} Table;

void table_init(Table *table);
void table_free(Table *table);
void table_copy_from(Table *from, Table *to);

bool table_set(Table *table, ObjString *key, Value value);

#endif
