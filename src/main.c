#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include "utils.h"

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

    return 1;
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
