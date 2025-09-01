#pragma once
#include "error.h"
#include "vm.h"

// Main runtime error function - never returns
// This function will either longjmp (REPL/test) or exit (script)
void slate_runtime_error(slate_vm* vm, ErrorKind kind,
                         const char* file, int line, int column,
                         const char* fmt, ...);

// Utility function that extracts debug location from values
// Uses the first available debug location from: b, a, or vm->current_debug
// This simplifies error reporting in opcodes
void slate_runtime_error_with_debug(slate_vm* vm, ErrorKind kind,
                                    value_t* a, value_t* b,
                                    const char* fmt, ...);