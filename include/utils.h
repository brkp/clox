#ifndef clox_utils_h
#define clox_utils_h

#include "common.h"

uint32_t hash_string(const char* key, int length);
char *read_file(const char *path);

#endif
