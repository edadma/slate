#include "vm.h"

vm_result op_pop_n(slate_vm* vm) {
    // Get the count of values to pop (operand)
    uint8_t count = *vm->ip;
    vm->ip++;
    
    // Pop and release the specified number of values
    for (int i = 0; i < count; i++) {
        if (vm->stack_top <= vm->stack) {
            printf("Runtime error: Stack underflow in POP_N\n");
            return VM_RUNTIME_ERROR;
        }
        value_t value = vm_pop(vm);
        vm_release(value);
    }
    
    return VM_OK;
}