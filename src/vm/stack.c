#include "vm.h"
#include "runtime_error.h"

// Stack operations
void vm_push(vm_t* vm, value_t value) {
    size_t current_size = vm->stack_top - vm->stack;
    if (!(current_size < vm->stack_capacity)) {
        printf("STACK OVERFLOW at size=%zu\n", current_size);
        slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, 
                           "Stack overflow: cannot push more values");
    }
    // Debug: Show every 10th push to track growth pattern
    // if (current_size > 0 && current_size % 10 == 0) {
    //     printf("STACK: %zu\n", current_size);
    // }
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