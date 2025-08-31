#include "vm.h"

vm_result op_add(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // String concatenation (if either operand is a string)
    if (a.type == VAL_STRING || b.type == VAL_STRING) {
        // Convert both to strings using helper function
        ds_string str_a = value_to_string_representation(vm, a);
        ds_string str_b = value_to_string_representation(vm, b);

        // Concatenate using DS library
        ds_string result = ds_append(str_a, str_b);
        vm_push(vm, make_string_ds_with_debug(result, a.debug));

        // Clean up temporary strings
        ds_release(&str_a);
        ds_release(&str_b);
    }
    // Array concatenation (if both operands are arrays)
    else if (a.type == VAL_ARRAY && b.type == VAL_ARRAY) {
        // Create new array for concatenation result
        da_array result_array = da_new(sizeof(value_t));

        // Add all elements from left array
        size_t a_len = da_length(a.as.array);
        for (size_t i = 0; i < a_len; i++) {
            value_t* elem = (value_t*)da_get(a.as.array, i);
            value_t retained_elem = vm_retain(*elem);
            da_push(result_array, &retained_elem);
        }

        // Add all elements from right array
        size_t b_len = da_length(b.as.array);
        for (size_t i = 0; i < b_len; i++) {
            value_t* elem = (value_t*)da_get(b.as.array, i);
            value_t retained_elem = vm_retain(*elem);
            da_push(result_array, &retained_elem);
        }

        vm_push(vm, make_array_with_debug(result_array, a.debug));
    }
    // Numeric addition - handle all numeric type combinations
    else if (is_number(a) && is_number(b)) {

        // int32 + int32 with overflow detection
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            int32_t result;
            if (di_add_overflow_int32(a.as.int32, b.as.int32, &result)) {
                vm_push(vm, make_int32_with_debug(result, a.debug));
            } else {
                // Overflow - promote to BigInt
                int64_t big_result = (int64_t)a.as.int32 + (int64_t)b.as.int32;
                di_int big = di_from_int64(big_result);
                vm_push(vm, make_bigint_with_debug(big, a.debug));
            }
        }
        // BigInt + BigInt
        else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
            di_int result = di_add(a.as.bigint, b.as.bigint);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        }
        // int32 + BigInt
        else if (a.type == VAL_INT32 && b.type == VAL_BIGINT) {
            di_int result = di_add_i32(b.as.bigint, a.as.int32);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        }
        // BigInt + int32
        else if (a.type == VAL_BIGINT && b.type == VAL_INT32) {
            di_int result = di_add_i32(a.as.bigint, b.as.int32);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        }
        // Mixed with floating point - convert to double
        else {
            double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
                : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                                 : a.as.number;
            double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
                : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                                 : b.as.number;
            vm_push(vm, make_number_with_debug(a_val + b_val, a.debug));
        }
    } else {
        // Find the first non-numeric operand for error location
        debug_location* error_debug = NULL;

        if (!is_number(a)) {
            // Left operand is the first non-numeric
            error_debug = a.debug;
        } else {
            // Right operand must be non-numeric
            error_debug = b.debug;
        }

        vm_runtime_error_with_values(vm, "Cannot add %s and %s", &a, &b, error_debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }

    // Clean up operands
    vm_release(a);
    vm_release(b);
    return VM_OK;
}