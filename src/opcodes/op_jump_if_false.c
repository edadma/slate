#include "vm.h"

vm_result op_jump_if_false(vm_t* vm) {
    uint16_t offset = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Pop condition from stack
    value_t condition = vm_pop(vm);
    
    // Check if condition is falsy
    if (is_falsy(condition)) {
        // Jump by the offset (relative jump)
        vm->ip += offset;
    }
    
    vm_release(condition);
    return VM_OK;
}