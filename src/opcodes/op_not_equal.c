#include "vm.h"

vm_result op_not_equal(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);
    int result = !values_equal(a, b);
    vm_push(vm, make_boolean(result));
    vm_release(a);
    vm_release(b);
    return VM_OK;
}