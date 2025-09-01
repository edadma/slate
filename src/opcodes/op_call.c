#include "vm.h"

vm_result op_call(slate_vm* vm) {
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

    // Get the callable value (now on top of stack)
    value_t callable = vm_pop(vm);
    // Handle bound methods (like array.map)
    if (callable.type == VAL_BOUND_METHOD) {
        bound_method_t* bound = callable.as.bound_method;

        // Prepare arguments: receiver + provided args
        value_t* full_args = malloc(sizeof(value_t) * (arg_count + 1));
        full_args[0] = bound->receiver; // First arg is receiver (the array)
        for (int i = 0; i < arg_count; i++) {
            full_args[i + 1] = args[i]; // Copy user-provided args
        }

        // Call the native function
        value_t result = bound->method(vm, arg_count + 1, full_args);
        vm_push(vm, result);

        free(full_args);
        if (args)
            free(args);
        return VM_OK;
    }

    // Handle closures (user functions) - set up call frame directly
    if (callable.type == VAL_CLOSURE || callable.type == VAL_FUNCTION) {
        function_t* func = NULL;
        closure_t* closure = NULL;
        
        if (callable.type == VAL_CLOSURE) {
            closure = callable.as.closure;
            func = closure->function;
        } else {
            func = callable.as.function;
            // Create a temporary closure wrapper
            closure = closure_create(func);
            if (!closure) {
                if (args) {
                    for (int i = 0; i < arg_count; i++) {
                        vm_release(args[i]);
                    }
                    free(args);
                }
                vm_release(callable);
                return VM_RUNTIME_ERROR;
            }
        }
        
        // Check argument count
        if (arg_count != func->parameter_count) {
            printf("Runtime error: Expected %d arguments but got %d\n", func->parameter_count, arg_count);
            if (callable.type == VAL_FUNCTION) {
                closure_destroy(closure);
            }
            if (args) {
                for (int i = 0; i < arg_count; i++) {
                    vm_release(args[i]);
                }
                free(args);
            }
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }
        
        // Check if we have room for another call frame
        if (vm->frame_count >= vm->frame_capacity) {
            printf("Runtime error: Stack overflow\n");
            if (callable.type == VAL_FUNCTION) {
                closure_destroy(closure);
            }
            if (args) {
                for (int i = 0; i < arg_count; i++) {
                    vm_release(args[i]);
                }
                free(args);
            }
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }
        
        // Push arguments onto the VM stack (they become the function's local variables)
        for (int i = 0; i < arg_count; i++) {
            vm_push(vm, args[i]);
        }
        
        // Set up new call frame
        call_frame* frame = &vm->frames[vm->frame_count++];
        frame->closure = closure;
        frame->ip = vm->ip; // Save return address (current IP)
        frame->slots = vm->stack_top - arg_count; // Point to function's arguments
        
        // Switch execution to the function
        vm->ip = func->bytecode;
        vm->bytecode = func->bytecode;
        
        if (args)
            free(args);
        vm_release(callable);
        return VM_OK;
    }

    // Handle native functions (built-ins)
    if (callable.type == VAL_NATIVE) {
        native_t builtin_func = (native_t)callable.as.native;
        value_t result = builtin_func(vm, arg_count, args);
        vm_push(vm, result);
        if (args)
            free(args);
        vm_release(callable);
        return VM_OK;
    }

    // Handle array indexing (arrays are callable with one integer argument)
    if (callable.type == VAL_ARRAY) {
        if (arg_count != 1) {
            printf("Runtime error: Array indexing requires exactly one argument\n");
            if (args) {
                for (int i = 0; i < arg_count; i++) {
                    vm_release(args[i]);
                }
                free(args);
            }
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }

        value_t index_val = args[0];
        if (index_val.type != VAL_INT32) {
            printf("Runtime error: Array index must be an integer\n");
            vm_release(args[0]);
            free(args);
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }

        int32_t index = index_val.as.int32;
        size_t array_length = da_length(callable.as.array);

        if (index < 0 || index >= array_length) {
            // Out of bounds - return null as error indicator
            vm_push(vm, make_null());
            vm_release(args[0]);
            free(args);
            vm_release(callable);
            return VM_OK;
        }

        // Get the element at the index
        value_t* element = (value_t*)da_get(callable.as.array, index);
        value_t result = vm_retain(*element);
        vm_push(vm, result);

        vm_release(args[0]);
        free(args);
        vm_release(callable);
        return VM_OK;
    }

    // Handle string indexing (strings are callable with one integer argument)
    if (callable.type == VAL_STRING) {
        if (arg_count != 1) {
            printf("Runtime error: String indexing requires exactly one argument\n");
            if (args) {
                for (int i = 0; i < arg_count; i++) {
                    vm_release(args[i]);
                }
                free(args);
            }
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }

        value_t index_val = args[0];
        if (index_val.type != VAL_INT32) {
            printf("Runtime error: String index must be an integer\n");
            vm_release(args[0]);
            free(args);
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }

        int32_t index = index_val.as.int32;
        size_t string_length = ds_length(callable.as.string);

        if (index < 0 || index >= string_length) {
            // Out of bounds - return null as error indicator
            vm_push(vm, make_null());
            vm_release(args[0]);
            free(args);
            vm_release(callable);
            return VM_OK;
        }

        // Get the character at the index
        char ch = callable.as.string[index];
        char ch_str[2] = {ch, '\0'};
        vm_push(vm, make_string(ch_str));

        vm_release(args[0]);
        free(args);
        vm_release(callable);
        return VM_OK;
    }

    // Handle class constructors (classes with factory functions)
    if (callable.type == VAL_CLASS) {
        class_t* cls = callable.as.class;
        if (cls->factory != NULL) {
            // Call the factory function to create an instance
            value_t result = cls->factory(args, arg_count);
            vm_push(vm, result);

            if (args) {
                for (int i = 0; i < arg_count; i++) {
                    vm_release(args[i]);
                }
                free(args);
            }
            vm_release(callable);
            return VM_OK;
        }
        // If no factory, fall through to error
    }

    printf("Runtime error: Value is not callable\n");
    if (args) {
        for (int i = 0; i < arg_count; i++) {
            vm_release(args[i]);
        }
        free(args);
    }
    vm_release(callable);
    return VM_RUNTIME_ERROR;
}
