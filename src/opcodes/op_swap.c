#include "vm.h"
#include "runtime_error.h"

vm_result op_swap(vm_t* vm) {
    // Stack: [a, b] -> [b, a]
    if (vm->stack_top < vm->stack + 2) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Stack underflow in SWAP");
        return VM_STACK_UNDERFLOW;
    }
    
    // Pop the top two values
    value_t b = vm_pop(vm);  // Top value
    value_t a = vm_pop(vm);  // Second value
    
    // Push them back in swapped order
    vm_push(vm, b);  // b goes to bottom
    vm_push(vm, a);  // a goes to top
    
    return VM_OK;
}