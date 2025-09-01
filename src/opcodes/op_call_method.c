#include "vm.h"

vm_result op_call_method(slate_vm* vm) {
    uint16_t arg_count = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Pop arguments into temporary array (they're on stack in reverse order)
    value_t* args = NULL;
    if (arg_count > 0) {
        args = malloc(sizeof(value_t) * arg_count);
        for (int i = arg_count - 1; i >= 0; i--) {
            args[i] = vm_pop(vm);
        }
    }

    // Get the method (function/closure) from stack
    value_t method = vm_pop(vm);
    
    // Get the receiver (implicit 'this' object) from stack
    value_t receiver = vm_pop(vm);

    // For method calls, we need to handle the receiver specially
    if (method.type == VAL_CLOSURE) {
        // Create new args array with receiver as first argument
        value_t* method_args = malloc(sizeof(value_t) * (arg_count + 1));
        method_args[0] = receiver;  // Receiver is first argument
        for (int i = 0; i < arg_count; i++) {
            method_args[i + 1] = args[i];  // Copy provided args
        }
        
        // Call the method with receiver + args
        value_t result = vm_call_function(vm, method, arg_count + 1, method_args);
        vm_push(vm, result);
        
        free(method_args);
        if (args) free(args);
        return VM_OK;
    } else if (method.type == VAL_NATIVE) {
        // For native methods, receiver is typically passed as first argument
        value_t* method_args = malloc(sizeof(value_t) * (arg_count + 1));
        method_args[0] = receiver;  // Receiver is first argument
        for (int i = 0; i < arg_count; i++) {
            method_args[i + 1] = args[i];  // Copy provided args
        }
        
        native_t native_func = (native_t)method.as.native;
        value_t result = native_func(vm, arg_count + 1, method_args);
        vm_push(vm, result);
        
        free(method_args);
        if (args) free(args);
        vm_release(method);
        return VM_OK;
    }

    // If method is not callable
    printf("Runtime error: Method is not callable\n");
    if (args) {
        for (int i = 0; i < arg_count; i++) {
            vm_release(args[i]);
        }
        free(args);
    }
    vm_release(method);
    vm_release(receiver);
    return VM_RUNTIME_ERROR;
}