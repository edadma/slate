#include "vm.h"
#include "runtime_error.h"

// Stack operations
void vm_push(vm_t* vm, value_t value) {
    if (!(vm->stack_top - vm->stack < vm->stack_capacity)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Stack overflow: cannot push more values");
    }
    *vm->stack_top = vm_retain(value);
    vm->stack_top++;
}

value_t vm_pop(vm_t* vm) {
    if (!(vm->stack_top > vm->stack)) {
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Stack underflow: cannot pop from empty stack");
    }
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(vm_t* vm, int distance) { return vm->stack_top[-1 - distance]; }