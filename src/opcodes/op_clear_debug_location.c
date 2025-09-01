#include "vm.h"

vm_result op_clear_debug_location(slate_vm* vm) {
    debug_location_free(vm->current_debug);
    vm->current_debug = NULL;
    return VM_OK;
}