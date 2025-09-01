#include "vm.h"

vm_result op_get_global(slate_vm* vm) {
    uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    // Get the current executing function from the current frame
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    if (name_constant >= current_func->constant_count) {
        printf("Runtime error: Constant index %d out of bounds (max %zu)\n", 
               name_constant, current_func->constant_count - 1);
        return VM_RUNTIME_ERROR;
    }
    value_t name_val = current_func->constants[name_constant];
    if (name_val.type != VAL_STRING) {
        printf("Runtime error: Global variable name must be a string\n");
        return VM_RUNTIME_ERROR;
    }
    char* name = name_val.as.string;
    
    // Check if we're in a function call and look for parameters first
    if (vm->frame_count > 0) {
        call_frame* current_frame = &vm->frames[vm->frame_count - 1];
        function_t* func = current_frame->closure->function;
        
        // Check if name matches a parameter
        for (size_t i = 0; i < func->parameter_count; i++) {
            if (strcmp(name, func->parameter_names[i]) == 0) {
                // Found parameter - get value from stack slot
                value_t param_value = current_frame->slots[i];
                vm_push(vm, param_value);
                return VM_OK;
            }
        }
    }
    
    // Fall through to global variable lookup
    value_t* stored_value = (value_t*)do_get(vm->globals, name);
    if (stored_value) {
        vm_push(vm, *stored_value);
    } else {
        printf("Runtime error: Undefined variable '%s'\n", name);
        return VM_RUNTIME_ERROR;
    }
    
    return VM_OK;
}