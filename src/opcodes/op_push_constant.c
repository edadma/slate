#include "vm.h"
#include "runtime_error.h"

vm_result op_push_constant(slate_vm* vm) {
    uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2; // Skip the operand bytes
    
    // Get the current executing function for constants
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    
    if (constant >= current_func->constant_count) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index %d out of bounds (max %zu)", constant, current_func->constant_count - 1);
    }
    
    // Create value with current debug info - use CURRENT function's constants
    value_t val = current_func->constants[constant];
    if (vm->current_debug) {
        val.debug = debug_location_copy(vm->current_debug);
    }
    vm_push(vm, val);
    return VM_OK;
}