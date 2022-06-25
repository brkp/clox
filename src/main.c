#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static char *read_file(const char *path) {
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

static int repl(void) {
    return 0;
}

static int run_file(const char *path) {
    VM vm;
    vm_init(&vm);

    char *source = read_file(path);
    if (source == NULL)
        exit(74);

    InterpretResult result = vm_interpret(&vm, source);

    free(source);
    vm_free(&vm);

    switch (result) {
        case INTERPRET_OK:            return 0;
        case INTERPRET_COMPILE_ERROR: return 65;
        case INTERPRET_RUNTIME_ERROR: return 70;
    }
}

int main(int argc, const char *argv[]) {
    switch (argc) {
        case 1: return repl();
        case 2: return run_file(argv[1]);

        default:
            fprintf(stderr, "usage: %s [path]\n", argv[0]);
            return 64;
    }
}
