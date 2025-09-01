#ifndef SLATE_OPCODES_H
#define SLATE_OPCODES_H

#include "vm.h"

// Individual opcode implementations
vm_result op_add(slate_vm* vm);
vm_result op_subtract(slate_vm* vm);
vm_result op_multiply(slate_vm* vm);
vm_result op_divide(slate_vm* vm);
vm_result op_mod(slate_vm* vm);
vm_result op_negate(slate_vm* vm);
vm_result op_equal(slate_vm* vm);
vm_result op_less(slate_vm* vm);
vm_result op_less_equal(slate_vm* vm);
vm_result op_greater(slate_vm* vm);
vm_result op_greater_equal(slate_vm* vm);
vm_result op_return(slate_vm* vm);
vm_result op_get_local(slate_vm* vm);
vm_result op_set_local(slate_vm* vm);
vm_result op_define_global(slate_vm* vm);
vm_result op_set_global(slate_vm* vm);
vm_result op_build_object(slate_vm* vm);
vm_result op_power(slate_vm* vm);
vm_result op_build_array(slate_vm* vm);
vm_result op_and(slate_vm* vm);
vm_result op_bitwise_and(slate_vm* vm);

// New opcodes extracted from vm.c
vm_result op_push_constant(slate_vm* vm);
vm_result op_get_global(slate_vm* vm);
vm_result op_get_property(slate_vm* vm);
vm_result op_call(slate_vm* vm);
vm_result op_closure(slate_vm* vm);
vm_result op_set_debug_location(slate_vm* vm);
vm_result op_push_null(slate_vm* vm);
vm_result op_push_undefined(slate_vm* vm);
vm_result op_push_true(slate_vm* vm);
vm_result op_push_false(slate_vm* vm);
vm_result op_pop(slate_vm* vm);
vm_result op_dup(slate_vm* vm);
vm_result op_set_result(slate_vm* vm);
vm_result op_clear_debug_location(slate_vm* vm);
vm_result op_halt(slate_vm* vm);

// Missing opcodes causing test failures
vm_result op_bitwise_or(slate_vm* vm);
vm_result op_bitwise_xor(slate_vm* vm);
vm_result op_bitwise_not(slate_vm* vm);
vm_result op_left_shift(slate_vm* vm);
vm_result op_right_shift(slate_vm* vm);
vm_result op_logical_right_shift(slate_vm* vm);
vm_result op_floor_div(slate_vm* vm);
vm_result op_increment(slate_vm* vm);
vm_result op_in(slate_vm* vm);
vm_result op_call_method(slate_vm* vm);
vm_result op_pop_n_preserve_top(slate_vm* vm);
vm_result op_build_range(slate_vm* vm);
vm_result op_jump_if_false(slate_vm* vm);

#endif // SLATE_OPCODES_H