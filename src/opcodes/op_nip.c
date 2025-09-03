#include "vm.h"
#include "runtime_error.h"

vm_result op_nip(vm_t* vm) {
    // Stack: [a, b] -> [b]
    // Remove the second item from the top, keeping the top item
    if (vm->stack_top < vm->stack + 2) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Stack underflow in NIP");
        return VM_STACK_UNDERFLOW;
    }
    
    // Pop the top value (keep it)
    value_t top = vm_pop(vm);
    
    // Pop and release the second value (discard it)
    value_t second = vm_pop(vm);
    vm_release(second);
    
    // Push the top value back
    vm_push(vm, top);
    
    return VM_OK;
}