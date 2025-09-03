#include "vm.h"
#include "module.h"

// Get export operation (for future use)
// This could be used for accessing module exports without importing them
vm_result op_get_export(vm_t* vm) {
    // This opcode is reserved for future implementation
    // It could be used for qualified access like "module.symbol"
    
    // For now, just return an error
    return VM_RUNTIME_ERROR;
}