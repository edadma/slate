#include "vm.h"
#include "runtime_error.h"

vm_result op_define_global(vm_t* vm) {
    // Pop the value to store and the variable name constant
    value_t value = vm_pop(vm);

    // Note: We allow undefined for variable declarations (var x;)
    // The restriction on undefined only applies to explicit assignments

    uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Get the current executing function from the current frame
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    if (name_constant >= current_func->constant_count) {
        vm_release(value);
        runtime_error(vm, "Constant index out of bounds in OP_DEFINE_GLOBAL");
        return VM_RUNTIME_ERROR;
    }
    
    value_t name_val = current_func->constants[name_constant];
    if (name_val.type != VAL_STRING) {
        vm_release(value);
        runtime_error(vm, "Global variable name must be a string");
        return VM_RUNTIME_ERROR;
    }

    // Store in globals object - we need to store a copy of the value
    value_t* stored_value = malloc(sizeof(value_t));
    if (!stored_value) {
        vm_release(value);
        runtime_error(vm, "Memory allocation failed");
        return VM_RUNTIME_ERROR;
    }
    *stored_value = value;

    // ds_string can be used directly as char* - no ds_cstr needed
    // do_set needs key, data pointer, and size
    do_set(vm->globals, name_val.as.string, stored_value, sizeof(value_t));
    return VM_OK;
}