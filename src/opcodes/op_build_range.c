#include "vm.h"
#include "runtime_error.h"

vm_result op_build_range(vm_t* vm) {
    uint16_t exclusive_flag = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;

    // Pop step, end, and start values from stack (in reverse order due to stack LIFO)
    value_t step = vm_pop(vm);
    value_t end = vm_pop(vm);
    value_t start = vm_pop(vm);

    // All values must be numbers
    if (!is_number(start) || !is_number(end) || !is_number(step)) {
        vm_runtime_error_with_values(vm, "Range bounds and step must be numbers, got %s, %s, and %s", &start, &end, start.debug);
        vm_release(start);
        vm_release(end);
        vm_release(step);
        return VM_RUNTIME_ERROR;
    }

    // Step cannot be zero
    if ((step.type == VAL_INT32 && step.as.int32 == 0) || 
        (step.type == VAL_FLOAT32 && step.as.float32 == 0.0f) ||
        (step.type == VAL_FLOAT64 && step.as.float64 == 0.0)) {
        runtime_error(vm, "Range step cannot be zero");
        vm_release(start);
        vm_release(end);
        vm_release(step);
        return VM_RUNTIME_ERROR;
    }

    // Auto-adjust step direction for ranges without explicit step
    if (start.type == VAL_INT32 && end.type == VAL_INT32 && step.type == VAL_INT32) {
        int32_t start_val = start.as.int32;
        int32_t end_val = end.as.int32;
        int32_t step_val = step.as.int32;
        
        // If step is 1 (default), auto-detect direction
        if (step_val == 1 && start_val > end_val) {
            // Auto-reverse: change step to -1 for reverse ranges
            vm_release(step);
            step = make_int32(-1);
        } else if (step_val != 1) {
            // Explicit step: check if step direction matches range direction
            if ((start_val < end_val && step_val < 0) || (start_val > end_val && step_val > 0)) {
                runtime_error(vm, "Range step direction doesn't match range direction");
                vm_release(start);
                vm_release(end);
                vm_release(step);
                return VM_RUNTIME_ERROR;
            }
        }
    }

    // Create the range value
    int exclusive = (exclusive_flag != 0);
    value_t range = make_range_with_debug(start, end, exclusive, step, start.debug);
    vm_push(vm, range);

    vm_release(start);
    vm_release(end);
    vm_release(step);
    return VM_OK;
}