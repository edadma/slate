#include "vm.h"

vm_result op_dup(slate_vm* vm) {
    value_t top = vm_peek(vm, 0);
    vm_push(vm, top);
    return VM_OK;
}