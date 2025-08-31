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
vm_result op_return(slate_vm* vm);
vm_result op_get_local(slate_vm* vm);
vm_result op_set_local(slate_vm* vm);

#endif // SLATE_OPCODES_H