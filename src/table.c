#include "memory.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void table_init(Table *table) {
    table->len = 0;
    table->cap = 0;
    table->entries = NULL;
}

void table_free(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->cap);
    table_init(table);
}

static Entry *find_entry(Entry *entries, int cap, ObjString *key) {
    uint32_t index = key->hash % cap;
    Entry *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == key || entry->key == NULL) {
            return entry;
        }
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return tombstone == NULL ? entry : tombstone;
            }
            else {
                if (tombstone == NULL) tombstone = entry;
            }
        }
        else if (entry->key == key) {
            return entry;
        }
        index = (index + 1) % cap;
    }
}

void table_copy_from(Table *from, Table *to) {
    for (int i = 0; i < from->cap; i++) {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL) {
            table_set(to, entry->key, entry->value);
        }
    }
}

static void adjust_capacity(Table *table, int capacity) {
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    for (int i = 0; i < table->cap; i++) {
        Entry *entry = &table->entries[i];
        if (entry == NULL) continue;

        Entry *dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
    }

    FREE_ARRAY(Entry, table->entries, table->cap);
    table->cap = capacity;
    table->entries = entries;
}

bool table_get(Table *table, ObjString *key, Value *value) {
    if (table->len == 0) return false;

    Entry *entry = find_entry(table->entries, table->cap, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

bool table_set(Table *table, ObjString *key, Value value) {
    if (table->len + 1 > table->cap * TABLE_MAX_LOAD) {
        int new_cap = GROW_CAPACITY(table->cap);
        adjust_capacity(table, new_cap);
    }

    Entry *entry = find_entry(table->entries, table->cap, key);
    bool is_new_key = entry->key == NULL;
    if (is_new_key && IS_NIL(entry->value)) table->len++;

    entry->key = key;
    entry->value = value;

    return is_new_key;
}

bool table_del(Table *table, ObjString *key) {
    if (table->len == 0) return false;

    Entry *entry = find_entry(table->entries, table->cap, key);
    if (entry->key == NULL) return false;

    // tombstone
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}
