# Improving `vm_call_function` with a Scoped Runner

The rogue opcodes (`81`, `71`) show that `vm_call_function` keeps the VM running *past* the function return, effectively
double-driving the interpreter. This happens because `vm_call_function` currently calls `vm_run(vm)` without any
stopping condition.

Option **B** introduces a **scoped runner** to fix the problem cleanly by running the VM **until the callee returns**,
then yielding control back to the caller.

---

## Concept

* Save the current call depth before entering the function.
* Run the VM until the stack depth returns to that saved level.
* Exit the runner and return the function's result.

This ensures:

* No double-running of caller bytecode.
* No random instruction pointer corruption.
* Clean, predictable execution.

---

## Implementation

### 1. Update the VM header

In `vm.h`:

```c
typedef enum {
    VM_OK = 0,
    VM_RUNTIME_ERROR,
    VM_YIELD
} vm_result;

vm_result vm_run_until_depth(slate_vm* vm, size_t target_depth);
```

---

### 2. Implement `vm_run_until_depth`

In `vm/core.c`, clone the main interpreter loop but stop once you return to the saved call depth:

```c
vm_result vm_run_until_depth(slate_vm* vm, size_t target_depth) {
    for (;;) {
        if (vm->frame_count == target_depth) {
            return VM_YIELD;
        }

        function_t* cur = vm->frames[vm->frame_count - 1].closure->function;
        if (vm->ip < cur->bytecode || vm->ip >= cur->bytecode + cur->bytecode_length) {
            vm_runtime_error_with_debug(vm, "IP out of range while executing %s",
                                        cur->name ? cur->name : "<anon>");
            return VM_RUNTIME_ERROR;
        }

        vm->current_instruction = vm->ip;
        opcode instruction = (opcode)*vm->ip++;

        switch (instruction) {
            case OP_RETURN: {
                vm_result r = op_return(vm);
                if (r != VM_OK) return r;
                if (vm->frame_count == target_depth) {
                    return VM_YIELD;
                }
                break;
            }

            /* Copy the entire big switch from vm_run into this function,
               or better, refactor vm_run to call a shared inner function
               so you don't duplicate logic. */

            default: {
                const char* name = opcode_name(instruction);
                fprintf(stderr, "Unimplemented opcode in vm_run_until_depth: %u (%s)\n",
                        (unsigned)instruction, name ? name : "unknown");
                return VM_RUNTIME_ERROR;
            }
        }
    }
}
```

---

### 3. Update `vm_call_function`

In `execution.c`, use the scoped runner:

```c
value_t vm_call_function(slate_vm* vm, value_t callable, int arg_count, value_t* args) {
    size_t saved_depth = vm->frame_count;

    // Existing frame setup code...

    vm_result result = vm_run_until_depth(vm, saved_depth);
    if (result == VM_RUNTIME_ERROR) {
        return make_undefined();
    }

    return vm->result;
}
```

---

## Debugging Aids

### 1. IP Bounds Check

Already included in the implementation, this will throw a clear error if the instruction pointer ever drifts out of
range.

### 2. Trace Logging

For deeper debugging, log every step:

```c
fprintf(stderr, "[depth=%zu] %s\n", vm->frame_count, opcode_name(instruction));
```

---

## Benefits

* **Safe Execution**: Prevents running caller code inside the callee runner.
* **No Hacks**: Eliminates temporary `HALT` insertion.
* **Easier Maintenance**: Unified way to run nested function calls.
* **Immediate Feedback**: Bounds check triggers clear runtime errors instead of silent corruption.

---

## Expected Behavior

After implementing this, the following code:

```slate
var f = (x) -> x + 4;
[3, 4, 5].map(f);
```

will correctly return:

```
[7, 8, 9]
```

with no random opcodes or invalid constant index errors.
