#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

// Function operations
function_t* function_create(const char* name) {
    function_t* function = malloc(sizeof(function_t));
    if (!function)
        return NULL;

    function->bytecode = NULL;
    function->bytecode_length = 0;
    function->constants = NULL;
    function->constant_count = 0;
    function->parameter_names = NULL;
    function->parameter_count = 0;
    function->local_count = 0;
    function->name = name ? strdup(name) : NULL;
    function->debug = NULL; // Initialize debug info

    return function;
}

void function_destroy(function_t* function) {
    if (!function)
        return;

    free(function->bytecode);

    for (size_t i = 0; i < function->constant_count; i++) {
        free_value(function->constants[i]);
    }
    free(function->constants);

    for (size_t i = 0; i < function->parameter_count; i++) {
        free(function->parameter_names[i]);
    }
    free(function->parameter_names);

    debug_info_destroy(function->debug);

    free(function->name);
    free(function);
}

closure_t* closure_create(function_t* function) {
    closure_t* closure = malloc(sizeof(closure_t));
    if (!closure)
        return NULL;

    closure->function = function;
    closure->upvalues = NULL;
    closure->upvalue_count = 0;

    return closure;
}

void closure_destroy(closure_t* closure) {
    if (!closure)
        return;

    function_destroy(closure->function);
    free(closure->upvalues);
    free(closure);
}