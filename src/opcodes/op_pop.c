#include "vm.h"

vm_result op_pop(slate_vm* vm) {
    vm_release(vm_pop(vm));
    return VM_OK;
}