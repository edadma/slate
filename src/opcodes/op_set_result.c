#include "vm.h"

vm_result op_set_result(slate_vm* vm) {
    value_t result = vm_pop(vm);
    vm->result = result;
    return VM_OK;
}