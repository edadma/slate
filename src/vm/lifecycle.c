#include "vm.h"
#include "memory.h"
#include "../opcodes/opcodes.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtins.h"
#include "codegen.h" // For debug_info functions
#include "datetime.h" // For date/time functions

#define STACK_MAX 256
#define FRAMES_MAX 64
#define CONSTANTS_MAX 256

// VM lifecycle functions
slate_vm* vm_create(void) {
    slate_vm* vm = malloc(sizeof(slate_vm));
    if (!vm)
        return NULL;

    vm->stack = malloc(sizeof(value_t) * STACK_MAX);
    vm->frames = malloc(sizeof(call_frame) * FRAMES_MAX);
    vm->constants = malloc(sizeof(value_t) * CONSTANTS_MAX);

    if (!vm->stack || !vm->frames || !vm->constants) {
        vm_destroy(vm);
        return NULL;
    }

    vm->stack_capacity = STACK_MAX;
    vm->frame_capacity = FRAMES_MAX;
    vm->constant_capacity = CONSTANTS_MAX;

    // Create globals object - bit_value will be stored as property values
    vm->globals = do_create(NULL); // No release function yet
    if (!vm->globals) {
        vm_destroy(vm);
        return NULL;
    }

    // Create function table - stores all defined functions with reference counting
    vm->functions = da_new(sizeof(function_t*)); // Store pointers to functions
    if (!vm->functions) {
        vm_destroy(vm);
        return NULL;
    }

    // Initialize built-in functions
    builtins_init(vm);

    // Initialize result register to undefined
    vm->result = make_undefined();

    // Initialize debug location
    vm->current_debug = NULL;

    // Initialize command line arguments (empty by default)
    vm->argc = 0;
    vm->argv = NULL;
    
    // Initialize error handling fields
    vm->context = CTX_SCRIPT;  // Default to script context
    vm->error.kind = ERR_NONE;
    vm->error.file = NULL;
    vm->error.line = 0;
    vm->error.column = 0;
    vm->error.message[0] = '\0';
    // Note: trap is initialized when needed via setjmp

    vm_reset(vm);
    return vm;
}

slate_vm* vm_create_with_args(int argc, char** argv) {
    slate_vm* vm = vm_create();
    if (vm) {
        vm->argc = argc;
        vm->argv = argv;
    }
    return vm;
}

void vm_destroy(slate_vm* vm) {
    if (!vm)
        return;

    // Free constants
    for (size_t i = 0; i < vm->constant_count; i++) {
        free_value(vm->constants[i]);
    }

    free(vm->stack);
    free(vm->frames);
    free(vm->constants);
    do_release(&vm->globals);
    
    // Release function table (functions handle their own ref counting)
    da_release(&vm->functions);

    // Release result register
    free_value(vm->result);

    // Clean up debug location
    debug_location_free(vm->current_debug);

    free(vm);
}

void vm_reset(slate_vm* vm) {
    if (!vm)
        return;

    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->constant_count = 0;
    vm->bytes_allocated = 0;
    vm->bytecode = NULL;
    vm->ip = NULL;
}