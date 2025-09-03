#include "vm.h"
#include "runtime_error.h"

vm_result op_rot(vm_t* vm) {
    // Stack: [a, b, c] -> [b, c, a]
    // Rotate the top three items: bottom item moves to top
    if (vm->stack_top < vm->stack + 3) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Stack underflow in ROT");
        return VM_STACK_UNDERFLOW;
    }
    
    // Pop the top three values
    value_t c = vm_pop(vm);  // Top
    value_t b = vm_pop(vm);  // Middle  
    value_t a = vm_pop(vm);  // Bottom
    
    // Push them back in rotated order: [b, c, a]
    vm_push(vm, b);  // b goes to bottom
    vm_push(vm, c);  // c goes to middle
    vm_push(vm, a);  // a goes to top
    
    return VM_OK;
}