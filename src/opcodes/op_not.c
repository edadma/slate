#include "vm.h"

vm_result op_not(vm_t* vm) {
    value_t a = vm_pop(vm);
    
    // Logical NOT: convert to boolean and negate
    bool result = is_falsy(a);
    
    vm_push(vm, make_boolean(result));
    vm_release(a);
    
    return VM_OK;
}