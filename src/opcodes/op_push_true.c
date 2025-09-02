#include "vm.h"

vm_result op_push_true(vm_t* vm) {
    vm_push(vm, make_boolean_with_debug(1, vm->current_debug));
    return VM_OK;
}