#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Error handling
void codegen_error(codegen_t* codegen, const char* message) {
    fprintf(stderr, "Codegen error: %s\n", message);
    codegen->had_error = 1;
}