#include "vm.h"
#include "runtime_error.h"

vm_result op_over(vm_t* vm) {
    // Stack: [a, b] -> [a, b, a]
    // Duplicate the second item to top
    if (vm->stack_top < vm->stack + 2) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Stack underflow in OVER");
        return VM_STACK_UNDERFLOW;
    }
    
    // Peek at the second item (without popping)
    value_t second = vm_peek(vm, 1);  // Second item from top
    
    // Push a copy of the second item to top
    vm_push(vm, second);
    
    return VM_OK;
}