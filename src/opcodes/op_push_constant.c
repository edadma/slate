#include "vm.h"
#include "runtime_error.h"

vm_result op_push_constant(vm_t* vm) {
    uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2; // Skip the operand bytes
    
    value_t val;
    
    // Context-aware constant pool access
    if (vm->frame_count == 0) {
        // Main program execution - use global constants
        if (constant >= vm->constant_count) {
            slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index %d out of bounds (max %zu)", constant, vm->constant_count - 1);
            return VM_RUNTIME_ERROR;
        }
        val = vm->constants[constant];
    } else {
        // Function execution - use function's own constant pool
        function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
        
        if (!current_func) {
            slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Current function is NULL");
            return VM_RUNTIME_ERROR;
        }
        
        if (constant >= current_func->constant_count) {
            slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index %d out of bounds (max %zu)", constant, current_func->constant_count - 1);
            return VM_RUNTIME_ERROR;
        }
        val = current_func->constants[constant];
    }
    
    // Create value with current debug info
    if (vm->current_debug) {
        val.debug = debug_location_copy(vm->current_debug);
    }
    vm_push(vm, val);
    return VM_OK;
}