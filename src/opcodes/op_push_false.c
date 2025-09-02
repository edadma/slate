#include "vm.h"

vm_result op_push_false(vm_t* vm) {
    vm_push(vm, make_boolean_with_debug(0, vm->current_debug));
    return VM_OK;
}