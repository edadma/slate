#include "codegen.h"
#include "runtime_error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Error handling
void codegen_error(codegen_t* codegen, const char* message) {
    if (codegen->vm && codegen->vm->context == CTX_TEST) {
        // Suppress error messages in test context, but still set error flag
    } else {
        fprintf(stderr, "Codegen error: %s\n", message);
    }
    codegen->had_error = 1;
}