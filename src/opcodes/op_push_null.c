#include "vm.h"

vm_result op_push_null(slate_vm* vm) {
    vm_push(vm, make_null_with_debug(vm->current_debug));
    return VM_OK;
}