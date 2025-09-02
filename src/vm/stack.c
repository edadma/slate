#include "vm.h"
#include "runtime_error.h"

// Stack operations
void vm_push(slate_vm* vm, value_t value) {
    if (!(vm->stack_top - vm->stack < vm->stack_capacity)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Stack overflow: cannot push more values");
    }
    *vm->stack_top = vm_retain(value);
    vm->stack_top++;
}

value_t vm_pop(slate_vm* vm) {
    if (!(vm->stack_top > vm->stack)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Stack underflow: cannot pop from empty stack");
    }
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(slate_vm* vm, int distance) { return vm->stack_top[-1 - distance]; }