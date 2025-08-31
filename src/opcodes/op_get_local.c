#include "vm.h"

vm_result op_get_local(slate_vm* vm) {
    uint8_t slot = *vm->ip++;
    call_frame* frame = &vm->frames[vm->frame_count - 1];
    vm_push(vm, frame->slots[slot]);
    return VM_OK;
}