#include "vm.h"
#include "runtime_error.h"

vm_result op_pop_n_preserve_top(vm_t* vm) {
    uint16_t count = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Preserve the top value
    value_t top = vm_pop(vm);
    
    // Pop and release N values from the stack
    for (uint16_t i = 0; i < count; i++) {
        if (vm->stack_top <= vm->stack) {
            // Stack underflow
            vm_push(vm, top); // Restore the top value
            runtime_error(vm, "Stack underflow in POP_N_PRESERVE_TOP");
            return VM_RUNTIME_ERROR;
        }
        value_t val = vm_pop(vm);
        vm_release(val);
    }
    
    // Push the preserved top value back
    vm_push(vm, top);
    
    return VM_OK;
}