#include "vm.h"

vm_result op_clear_debug_location(vm_t* vm) {
    debug_location_free(vm->current_debug);
    vm->current_debug = NULL;
    return VM_OK;
}