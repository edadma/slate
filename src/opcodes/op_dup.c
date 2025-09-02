#include "vm.h"

vm_result op_dup(vm_t* vm) {
    value_t top = vm_peek(vm, 0);
    vm_push(vm, top);
    return VM_OK;
}