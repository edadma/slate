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

// Save complete VM state
static void vm_save_state(vm_t* vm, vm_call_state* state) {
    state->bytecode = vm->bytecode;
    state->ip = vm->ip;
    state->current_instruction = vm->current_instruction;
    state->stack_top = vm->stack_top;
    state->frame_count = vm->frame_count;
    state->constants = vm->constants;
    state->constant_count = vm->constant_count;
    state->current_module = vm->current_module;
    state->result = vm->result;
}

// Restore complete VM state
static void vm_restore_state(vm_t* vm, const vm_call_state* state) {
    vm->bytecode = state->bytecode;
    vm->ip = state->ip;
    vm->current_instruction = state->current_instruction;
    vm->stack_top = state->stack_top;
    vm->frame_count = state->frame_count;
    vm->constants = state->constants;
    vm->constant_count = state->constant_count;
    vm->current_module = state->current_module;
    vm->result = state->result;
}

// Safe function calling with complete VM state isolation
value_t vm_call_slate_function_safe(vm_t* vm, value_t callable, int arg_count, value_t* args) {
    // Only handle user-defined functions (closures/functions)
    if (callable.type == VAL_NATIVE) {
        // Native functions can be called directly without state isolation
        native_t func = (native_t)callable.as.native;
        return func(vm, arg_count, args);
    }
    
    if (callable.type != VAL_CLOSURE && callable.type != VAL_FUNCTION) {
        return make_undefined();
    }
    
    // Get function reference
    function_t* func = NULL;
    closure_t* closure = NULL;
    int created_closure = 0;
    
    if (callable.type == VAL_CLOSURE) {
        closure = callable.as.closure;
        func = closure->function;
    } else if (callable.type == VAL_FUNCTION) {
        func = callable.as.function;
        closure = closure_create(func);
        created_closure = 1;
    }
    
    // Validate argument count
    if (arg_count < func->parameter_count) {
        if (created_closure) closure_destroy(closure);
        return make_undefined();
    }
    
    // Use only the required number of arguments
    int actual_arg_count = (arg_count > func->parameter_count) ? func->parameter_count : arg_count;
    
    // Check frame capacity
    if (vm->frame_count >= vm->frame_capacity) {
        if (created_closure) closure_destroy(closure);
        return make_undefined();
    }
    
    // Save complete VM state (stack allocated)
    vm_call_state saved_state;
    vm_save_state(vm, &saved_state);
    
    // Push function arguments onto stack
    for (int i = 0; i < actual_arg_count; i++) {
        vm_push(vm, args[i]);
    }
    
    // Set up isolated execution context for the function
    call_frame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = vm->ip; // Save return address 
    frame->slots = vm->stack_top - actual_arg_count;
    
    // Switch to function's execution context
    vm->bytecode = func->bytecode;
    vm->constants = func->constants;
    vm->constant_count = func->constant_count;
    vm->ip = func->bytecode;
    vm->current_instruction = func->bytecode;
    
    // Set module context if closure has one
    if (closure->module) {
        vm->current_module = closure->module;
    }
    
    // Execute the function
    vm_result result = vm_run(vm);
    
    // Capture return value before restoring state
    value_t return_value = make_undefined();
    if (result == VM_OK) {
        return_value = vm->result;
    }
    
    // Restore complete VM state
    vm_restore_state(vm, &saved_state);
    
    // Clean up
    if (created_closure) {
        closure_destroy(closure);
    }
    
    return return_value;
}