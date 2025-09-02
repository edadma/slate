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

// Global VM pointer for library assert access
vm_t* g_current_vm = NULL;

// VM lifecycle functions
vm_t* vm_create(void) {
    vm_t* vm = malloc(sizeof(vm_t));
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
    
    // Create global immutability tracking object
    vm->global_immutability = do_create(NULL); // No release function needed for bools
    if (!vm->global_immutability) {
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

    // Set global VM pointer for library assert access
    g_current_vm = vm;

    vm_reset(vm);
    return vm;
}

vm_t* vm_create_with_args(int argc, char** argv) {
    vm_t* vm = vm_create();
    if (vm) {
        vm->argc = argc;
        vm->argv = argv;
    }
    return vm;
}

void vm_destroy(vm_t* vm) {
    if (!vm)
        return;

    // Clear global VM pointer if this is the current VM
    if (vm == g_current_vm) {
        g_current_vm = NULL;
    }

    // Free constants
    for (size_t i = 0; i < vm->constant_count; i++) {
        free_value(vm->constants[i]);
    }

    free(vm->stack);
    free(vm->frames);
    free(vm->constants);
    do_release(&vm->globals);
    do_release(&vm->global_immutability);
    
    // Release function table (functions handle their own ref counting)
    da_release(&vm->functions);

    // Release result register
    free_value(vm->result);

    // Clean up debug location
    debug_location_free(vm->current_debug);

    free(vm);
}

void vm_reset(vm_t* vm) {
    if (!vm)
        return;

    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->constant_count = 0;
    vm->bytes_allocated = 0;
    vm->bytecode = NULL;
    vm->ip = NULL;
}