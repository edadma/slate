#include "vm.h"

vm_result op_and(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Logical AND: if a is falsy, return a, otherwise return b
    if (is_falsy(a)) {
        vm_push(vm, a);
        vm_release(b);
    } else {
        vm_push(vm, b);
        vm_release(a);
    }
    
    return VM_OK;
}