#include "vm.h"

vm_result op_push_null(vm_t* vm) {
    vm_push(vm, make_null_with_debug(vm->current_debug));
    return VM_OK;
}