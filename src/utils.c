#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

uint32_t hash_string(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

char *read_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "couldn't open file '%s'\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "not enough memory to read '%s'\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "couldn't read file '%s'\n", path);
        fclose(file); free(buffer);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

