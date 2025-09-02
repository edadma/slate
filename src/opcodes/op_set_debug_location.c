#include "vm.h"

vm_result op_set_debug_location(vm_t* vm) {
    uint16_t constant_index = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    uint8_t line = *vm->ip++;
    uint8_t column = *vm->ip++;
    
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    if (constant_index < current_func->constant_count) {
        value_t source_val = current_func->constants[constant_index];
        if (source_val.type == VAL_STRING) {
            debug_location_free(vm->current_debug);
            vm->current_debug = debug_location_create(line, column, source_val.as.string);
        }
    }
    return VM_OK;
}