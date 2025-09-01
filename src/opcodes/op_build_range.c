#include "vm.h"

vm_result op_build_range(slate_vm* vm) {
    uint16_t exclusive_flag = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Pop end and start values from stack
    value_t end = vm_pop(vm);
    value_t start = vm_pop(vm);

    // Both start and end must be numbers
    if (!is_number(start) || !is_number(end)) {
        vm_runtime_error_with_values(vm, "Range bounds must be numbers, got %s and %s", &start, &end, start.debug);
        vm_release(start);
        vm_release(end);
        return VM_RUNTIME_ERROR;
    }

    // Create the range value
    int exclusive = (exclusive_flag != 0);
    value_t range = make_range_with_debug(start, end, exclusive, start.debug);
    vm_push(vm, range);

    vm_release(start);
    vm_release(end);
    return VM_OK;
}