#include "vm.h"

vm_result op_set_result(vm_t* vm) {
    value_t result = vm_pop(vm);
    vm->result = result;
    return VM_OK;
}