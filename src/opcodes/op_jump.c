#include "vm.h"

vm_result op_jump(vm_t* vm) {
    uint16_t offset = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Unconditional jump by the offset
    // The offset is stored as a 16-bit value but we need to handle it as signed
    // for backward jumps (negative offsets)
    int16_t signed_offset = (int16_t)offset;
    vm->ip += signed_offset;
    
    return VM_OK;
}