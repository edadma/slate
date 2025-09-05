#include "vm.h"
#include <assert.h>

// Core VM execution function - executes a function with the given closure
// Assumes the VM is already set up with proper stack state and call frame

// Helper function to call functions from C code (for array methods, etc.)
value_t vm_call_function(vm_t* vm, value_t callable, int arg_count, value_t* args) {
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

// Helper function for calling Slate functions from C code
// Now uses the shared VM instead of creating isolated VMs
value_t vm_call_slate_function_from_c(vm_t* vm, value_t callable, int arg_count, value_t* args) {
    // Simply use the regular vm_call_function - no more VM isolation needed
    return vm_call_function(vm, callable, arg_count, args);
}