#include "vm.h"
#include <assert.h>

// Core VM execution function - executes a function with the given closure
// Assumes the VM is already set up with proper stack state and call frame

// Helper function to call functions from C code (for array methods, etc.)
value_t vm_call_function(slate_vm* vm, value_t callable, int arg_count, value_t* args) {
    if (callable.type == VAL_NATIVE) {
        // Native functions are easy - just call them directly
        native_t func = (native_t)callable.as.native;
        return func(vm, arg_count, args);
    }
    
    if (callable.type != VAL_CLOSURE && callable.type != VAL_FUNCTION) {
        // Not callable - return undefined
        return make_undefined();
    }
    
    // For user-defined functions, be flexible with argument count
    // If function expects fewer arguments than provided, just pass the first N arguments
    function_t* func = NULL;
    if (callable.type == VAL_CLOSURE) {
        func = callable.as.closure->function;
    } else if (callable.type == VAL_FUNCTION) {
        func = callable.as.function;
    }
    
    int actual_arg_count = (arg_count > func->parameter_count) ? func->parameter_count : arg_count;
           
    // Only fail if we don't have enough arguments
    if (arg_count < func->parameter_count) {
        return make_undefined();
    }
    
    // Use the real VM execution infrastructure for proper closure execution
    closure_t* closure = NULL;
    int created_closure = 0;
    
    if (callable.type == VAL_CLOSURE) {
        closure = callable.as.closure;
    } else if (callable.type == VAL_FUNCTION) {
        closure = closure_create(func);
        created_closure = 1;
    }
    
    // Set up call frame on the current VM
    if (vm->frame_count >= vm->frame_capacity) {
        if (created_closure) closure_destroy(closure);
        return make_undefined();
    }
    
    // Save current VM state
    size_t saved_stack_size = vm->stack_top - vm->stack;
    uint8_t* saved_ip = vm->ip;
    uint8_t* saved_bytecode = vm->bytecode;
    size_t saved_frame_count = vm->frame_count;
    
    // Push arguments onto VM stack
    for (int i = 0; i < actual_arg_count; i++) {
        vm_push(vm, args[i]);
    }
    
    // Set up call frame
    call_frame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = saved_ip;
    frame->slots = vm->stack_top - actual_arg_count;
    
    // Switch execution context to function
    vm->ip = func->bytecode;
    vm->bytecode = func->bytecode;
    
    // Execute the function using our core execution loop
    vm_result result = vm_run(vm);
    
    value_t return_value = make_undefined();
    if (result == VM_OK) {
        // The function result is always in vm->result after OP_RETURN
        return_value = vm->result;
    }
    
    // Restore VM state
    vm->stack_top = vm->stack + saved_stack_size;
    // Only restore IP if we're not being called from bytecode execution
    // When called from bytecode (op_call), we should NOT restore IP
    // as the calling bytecode needs to continue from where it left off
    if (saved_frame_count == 0) {
        // Called from C code - restore IP
        vm->ip = saved_ip;
    }
    // Otherwise leave IP as set by OP_RETURN for proper bytecode continuation
    vm->bytecode = saved_bytecode;
    vm->frame_count = saved_frame_count;
    
    // Clean up
    if (created_closure) {
        closure_destroy(closure);
    }
    
    return return_value;
}

// Helper function for calling Slate functions from C code using isolated VM execution
// This avoids instruction pointer conflicts when calling closures from native functions
value_t vm_call_slate_function_from_c(slate_vm* vm, value_t callable, int arg_count, value_t* args) {
    // Handle native functions the same way
    if (callable.type == VAL_NATIVE) {
        native_t func = (native_t)callable.as.native;
        return func(vm, arg_count, args);
    }
    
    if (callable.type != VAL_CLOSURE && callable.type != VAL_FUNCTION) {
        // Not callable - return undefined
        return make_undefined();
    }
    
    // Get the function from the callable
    function_t* func = NULL;
    if (callable.type == VAL_CLOSURE) {
        func = callable.as.closure->function;
    } else if (callable.type == VAL_FUNCTION) {
        func = callable.as.function;
    }
    
    // Validate argument count
    if (arg_count < func->parameter_count) {
        return make_undefined();
    }
    
    // Create isolated VM for closure execution
    slate_vm* isolated_vm = vm_create();
    if (!isolated_vm) {
        return make_undefined();
    }
    
    // Copy shared state from original VM
    // Share the same globals (functions need access to global variables)
    isolated_vm->globals = vm->globals;
    
    // Share the same function table (functions may call other functions)
    isolated_vm->functions = vm->functions;
    
    // Set up call frame with arguments (similar to original vm_call_function)
    int actual_arg_count = (arg_count > func->parameter_count) ? func->parameter_count : arg_count;
    
    // Push arguments onto isolated VM stack
    for (int i = 0; i < actual_arg_count; i++) {
        vm_push(isolated_vm, args[i]);
    }
    
    // Set up call frame
    if (isolated_vm->frame_count >= isolated_vm->frame_capacity) {
        return make_undefined();
    }
    
    call_frame* frame = &isolated_vm->frames[isolated_vm->frame_count++];
    closure_t* closure = closure_create(func);
    frame->closure = closure;
    frame->ip = NULL; // Not used for isolated execution
    frame->slots = isolated_vm->stack_top - actual_arg_count;
    
    // Switch execution context to function
    isolated_vm->ip = func->bytecode;
    isolated_vm->bytecode = func->bytecode;
    
    // Execute the function using the core VM loop
    vm_result result = vm_run(isolated_vm);
    
    // Clean up closure
    closure_destroy(closure);
    
    value_t return_value = make_undefined();
    if (result == VM_OK) {
        // Get the return value from the isolated VM
        return_value = isolated_vm->result;
        // Retain the value since we're taking it out of the isolated VM
        return_value = vm_retain(return_value);
    }
    
    // Clean up isolated VM (but don't destroy shared globals/functions)
    // Temporarily clear shared references so vm_destroy doesn't free them
    isolated_vm->globals = do_create(NULL);  // Empty object to avoid double-free
    isolated_vm->functions = da_new(sizeof(function_t*));  // Empty array to avoid double-free
    
    vm_destroy(isolated_vm);
    
    return return_value;
}