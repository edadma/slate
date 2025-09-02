#include "vm.h"
#include "runtime_error.h"

vm_result op_pop_n(vm_t* vm) {
    // Get the count of values to pop (operand)
    uint8_t count = *vm->ip;
    vm->ip++;
    
    // Pop and release the specified number of values
    for (int i = 0; i < count; i++) {
        if (vm->stack_top <= vm->stack) {
            slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Stack underflow in POP_N");
        }
        value_t value = vm_pop(vm);
        vm_release(value);
    }
    
    return VM_OK;
}