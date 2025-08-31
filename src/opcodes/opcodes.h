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

#endif // SLATE_OPCODES_H