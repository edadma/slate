#include "vm.h"

vm_result op_pop(vm_t* vm) {
    vm_release(vm_pop(vm));
    return VM_OK;
}