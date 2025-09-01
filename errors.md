# Error Handling Plan for Slate

This document outlines a consistent and extensible error-handling system for the **Slate interpreter**. It uses
`setjmp`/`longjmp` for basic exception-like flow, integrates with `value_t`, and provides hooks for future expansion
into a richer exception system.

---

## 1. Core Error Type

```c
// error.h
#pragma once
#include <stdbool.h>

typedef enum {
  ERR_NONE = 0,
  ERR_OOM,
  ERR_SYNTAX,
  ERR_TYPE,
  ERR_REFERENCE,
  ERR_RANGE,
  ERR_IO,
  ERR_ASSERT,
} ErrorKind;

typedef struct {
  ErrorKind kind;
  int line, column;
  const char* file;
  char message[256];
} SlateError;
```

Each `VM` keeps track of its current error state:

```c
// in vm.h
typedef struct ErrorFrame {
  jmp_buf jb;
  struct ErrorFrame* prev;
} ErrorFrame;

typedef struct VM {
  ...
  SlateError error;
  ErrorFrame* err_stack;
  int cleanup_count;
  void (*cleanup_fns[32])(struct VM*, void*);
  void* cleanup_args[32];
} VM;
```

Utility to set the error:

```c
void vm_error_set(VM* vm, ErrorKind kind, int line, int col,
                  const char* file, const char* fmt, ...);
```

---

## 2. TRY/THROW/CATCH Macros

These macros form the basic error-handling API:

```c
#define VM_TRY(vm) \
  for (ErrorFrame _ef, *_efprev = (vm_push_error_frame((vm), &_ef), (ErrorFrame*)0); \
       _efprev == 0; vm_pop_error_frame(vm), _efprev = (ErrorFrame*)1) \
    if (setjmp(_ef.jb) == 0)

#define VM_CATCH else

#define VM_THROW(vm, kind, line, col, file, ...) do { \
  vm_error_set((vm), (kind), (line), (col), (file), __VA_ARGS__); \
  vm_run_cleanups(vm); \
  longjmp((vm)->err_stack->jb, 1); \
} while (0)

#define VM_THROWF(vm, kind, ...) VM_THROW((vm), (kind), -1, -1, NULL, __VA_ARGS__)
```

---

## 3. Cleanup Handling

`longjmp` skips normal stack unwinding, so a **cleanup stack** ensures resources are released:

```c
void vm_on_unwind(VM* vm, void (*fn)(VM*, void*), void* arg);

// Example for refcounted values
static void release_value_cleanup(VM* vm, void* p) {
  value_t* v = (value_t*)p;
  if (v) value_release(vm, *v);
}

static inline void vm_autorelease_value_on_unwind(VM* vm, value_t* v) {
  vm_on_unwind(vm, release_value_cleanup, v);
}
```

---

## 4. Integration Points

### 4.1 VM Entry Points

Wrap public entry points:

```c
bool slate_run(VM* vm, Chunk* chunk) {
  bool ok = true;
  VM_TRY(vm) {
    vm_execute(vm, chunk);
  } VM_CATCH {
    vm_reset_to_safe_state(vm);
    fprintf(stderr, "%s: %s\n", error_kind_name(vm->error.kind), vm->error.message);
    ok = false;
  }
  return ok;
}
```

### 4.2 Bytecode Operations

Use small macros for common checks:

```c
#define REQUIRE_CALLABLE(vm, v) do { \
  if (!value_is_callable(v)) \
    VM_THROWF(vm, ERR_TYPE, "Value is not callable"); \
} while (0)

#define CHECK_BOUNDS(vm, idx, len) do { \
  if ((idx) < 0 || (idx) >= (len)) \
    VM_THROWF(vm, ERR_RANGE, "Index %d out of bounds [0..%d)", (idx), (len)); \
} while (0)
```

### 4.3 Native Functions

Example:

```c
value_t native_array_push(VM* vm, int argc, value_t* argv) {
  REQUIRE_ARGC(vm, 2);
  value_t arr = argv[0];
  if (!is_array(arr))
    VM_THROWF(vm, ERR_TYPE, "array.push: receiver is not an array");

  value_t elem = argv[1];
  array_push(vm, arr, elem);
  return elem;
}
```

---

## 5. Parser and Lexer

Convert error exits into `VM_THROW` calls for consistency:

```c
static Expr* parse_primary(Parser* p) {
  if (match(p, TOKEN_RIGHT_PAREN))
    VM_THROW(&(p->vm), ERR_SYNTAX, token_line(p), token_col(p), p->filename,
             "Unexpected ')'");
  ...
}
```

---

## 6. Resetting VM State

After a throw, ensure the VM is clean:

```c
void vm_reset_to_safe_state(VM* vm) {
  // clear stacks, frames, and temporaries
  vm_run_cleanups(vm);
  // additional resets as needed
}
```

---

## 7. Migration Plan

1. Add the `SlateError` structure and macros.
2. Wrap entry points with `VM_TRY`/`VM_CATCH`.
3. Replace `fprintf`/`exit` calls with `VM_THROW`.
4. Use cleanup registration for refcounted objects.
5. Add basic tests for type errors, range errors, syntax errors, etc.
6. Centralize REPL error reporting.

---

## 8. Benefits

* **Consistency** – All errors handled through a single mechanism.
* **Memory Safety** – Cleanup stack ensures resources aren’t leaked.
* **Extensibility** – Easy to build user-level `try/catch/finally` later.
* **Debugging** – Unified error messages and easy tracing.

---

## 9. Unity Test Framework Integration

The Unity C test framework (`unity.h`) uses its own `setjmp/longjmp` protection (`TEST_PROTECT()` / `RUN_TEST`). To make
Slate’s error system play nicely with Unity:

### 9.1 Guiding Principles

1. **Don’t longjmp past Unity’s frames** from outside Unity’s `TEST_PROTECT()` boundary. Always throw within a region
   the test harness is protecting (either Unity’s protection or your own `VM_TRY`).
2. **Keep one reporting path.** Tests should assert on `vm->error` (kind, message, location) after a failure.
3. **Make throws testable.** Provide helpers that run code and capture errors without killing the test process.

### 9.2 Test-Safe Execution Helpers

Add a small facade used only in tests to run bytecode/scripts and return whether an error occurred.

```c
// test_support.h
#pragma once
#include "vm.h"
#include "error.h"

// Execute a chunk and return true on success, false on error.  
// Never let exceptions escape beyond this function; it consumes the throw.
static inline bool slate_test_run(VM* vm, Chunk* chunk) {
  bool ok = true;
  VM_TRY(vm) {
    vm_execute(vm, chunk);
  } VM_CATCH {
    vm_reset_to_safe_state(vm);
    ok = false;
  }
  return ok;
}

// Compile+run a source string; fill out error if any.
bool slate_test_eval_source(VM* vm, const char* filename, const char* src);
```

Implementation sketch:

```c
// test_support.c
#include "test_support.h"
#include "compiler.h"   // whatever produces Chunk*

bool slate_test_eval_source(VM* vm, const char* filename, const char* src) {
  Chunk* chunk = compile_source(vm, filename, src); // may VM_THROW
  if (!chunk) return false; // if your compiler returns NULL on error
  return slate_test_run(vm, chunk);
}
```

### 9.3 Unity Assertions for Errors

Provide assertion helpers to keep tests terse:

```c
// test_asserts.h
#pragma once
#include "unity.h"
#include "vm.h"
#include "error.h"

static inline void ASSERT_SlateErrorKind(VM* vm, ErrorKind expected) {
  TEST_ASSERT_EQUAL_INT_MESSAGE(expected, vm->error.kind, "ErrorKind mismatch");
}

static inline void ASSERT_SlateErrorMsgContains(VM* vm, const char* needle) {
  TEST_ASSERT_NOT_NULL(needle);
  TEST_ASSERT_NOT_NULL_MESSAGE(vm->error.message, "No error message present");
  TEST_ASSERT_NOT_EQUAL(-1, (int)strstr(vm->error.message, needle) - (int)vm->error.message);
}

static inline void ASSERT_SlateErrorLoc(VM* vm, int line, int col) {
  TEST_ASSERT_EQUAL_INT(line, vm->error.line);
  TEST_ASSERT_EQUAL_INT(col, vm->error.column);
}
```

### 9.4 Example Unity Tests

**TypeError on calling a number**

```c
#include "unity.h"
#include "test_support.h"

void test_calling_number_throws_type_error(void) {
  VM vm; vm_init(&vm);
  const char* src = "(123)(5)"; // calling a number

  bool ok = slate_test_eval_source(&vm, "repl", src);
  TEST_ASSERT_FALSE(ok);
  ASSERT_SlateErrorKind(&vm, ERR_TYPE);
  ASSERT_SlateErrorMsgContains(&vm, "not callable");
}
```

**RangeError on array index**

```c
void test_array_oob_throws_range_error(void) {
  VM vm; vm_init(&vm);
  const char* src = "var a=[1,2]; a[5]";

  bool ok = slate_test_eval_source(&vm, "repl", src);
  TEST_ASSERT_FALSE(ok);
  ASSERT_SlateErrorKind(&vm, ERR_RANGE);
}
```

**SyntaxError with location**

```c
void test_syntax_error_reports_location(void) {
  VM vm; vm_init(&vm);
  const char* src = "def f( {"; // bad paren

  bool ok = slate_test_eval_source(&vm, "file.slate", src);
  TEST_ASSERT_FALSE(ok);
  ASSERT_SlateErrorKind(&vm, ERR_SYNTAX);
  // Expect a sane 1-based line/column from your lexer
  ASSERT_SlateErrorLoc(&vm, 1, 7);
}
```

### 9.5 Using Unity’s TEST\_PROTECT (Optional)

If you prefer to let Unity be the outer guard, you can structure each test like this to catch unexpected longjmps:

```c
void test_whatever(void) {
  if (TEST_PROTECT()) {
    // Arrange + Act
    bool ok = slate_test_eval_source(&vm, "repl", code);
    // Assert
    TEST_ASSERT_FALSE(ok);
  }
  // If a VM_THROW escaped beyond slate_test_eval_source, Unity will flag the test as failed.
}
```

### 9.6 Forcing Specific Failures (e.g., OOM)

Inject a test allocator to simulate `ERR_OOM` deterministically:

```c
// vm_alloc.h
typedef void* (*vm_malloc_fn)(void* ud, size_t);
typedef void  (*vm_free_fn)(void* ud, void*);

void vm_set_allocator(VM* vm, vm_malloc_fn m, vm_free_fn f, void* ud);
```

In a test, install an allocator that returns `NULL` after N calls to trigger `VM_THROWF(vm, ERR_OOM, ...)` and assert on
`vm->error`.

---

## 10. Example Workflow

```c
VM_TRY(vm) {
  value_t result = vm_call_function(vm, func, args, argc);
  printf("Result: %s
", value_to_string(result));
} VM_CATCH {
  fprintf(stderr, "Error: %s
", vm->error.message);
}
```

---
