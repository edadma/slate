#include "vm.h"

vm_result op_loop(vm_t* vm) {
    // Get the loop offset (16-bit operand)
    uint16_t offset = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    
    // Jump backward by the specified offset
    vm->ip -= offset;
    
    return VM_OK;
}