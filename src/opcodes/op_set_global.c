#include "vm.h"

vm_result op_set_global(slate_vm* vm) {
    // Pop the value to store and get the variable name constant
    value_t value = vm_pop(vm);

    // Check if trying to assign undefined (not a first-class value)
    if (value.type == VAL_UNDEFINED) {
        vm_runtime_error_with_debug(vm, "Cannot assign 'undefined' - it is not a value");
        vm_release(value);
        return VM_RUNTIME_ERROR;
    }

    uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Get the current executing function from the current frame
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    if (name_constant >= current_func->constant_count) {
        vm_runtime_error_with_debug(vm, "Constant index out of bounds in OP_SET_GLOBAL");
        vm_release(value);
        return VM_RUNTIME_ERROR;
    }

    value_t name_val = current_func->constants[name_constant];
    if (name_val.type != VAL_STRING) {
        vm_runtime_error_with_debug(vm, "Global variable name must be a string");
        vm_release(value);
        return VM_RUNTIME_ERROR;
    }

    // Check if variable exists
    value_t* stored_value = (value_t*)do_get(vm->globals, name_val.as.string);
    if (stored_value) {
        // Release the old value first (proper reference counting)
        vm_release(*stored_value);

        // Update with new value (already popped from stack, so we own it)
        *stored_value = value;
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Undefined variable '%s'", name_val.as.string);
        vm_runtime_error_with_debug(vm, error_msg);
        vm_release(value);
        return VM_RUNTIME_ERROR;
    }
    return VM_OK;
}