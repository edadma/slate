#include "vm.h"
#include <assert.h>

// Stack operations
void vm_push(slate_vm* vm, value_t value) {
    assert(vm->stack_top - vm->stack < vm->stack_capacity);
    *vm->stack_top = vm_retain(value);
    vm->stack_top++;
}

value_t vm_pop(slate_vm* vm) {
    assert(vm->stack_top > vm->stack);
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(slate_vm* vm, int distance) { return vm->stack_top[-1 - distance]; }