#include "vm.h"

vm_result op_push_undefined(vm_t* vm) {
    vm_push(vm, make_undefined_with_debug(vm->current_debug));
    return VM_OK;
}