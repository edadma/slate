#include "runtime_error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External reference to global VM pointer (defined in vm/lifecycle.c)
extern vm_t* g_current_vm;

// Helper function to get error kind name
static const char* error_kind_name(ErrorKind k) {
    switch (k) {
    case ERR_OOM:
        return "OutOfMemoryError";
    case ERR_SYNTAX:
        return "SyntaxError";
    case ERR_TYPE:
        return "TypeError";
    case ERR_REFERENCE:
        return "ReferenceError";
    case ERR_RANGE:
        return "RangeError";
    case ERR_IO:
        return "IOError";
    case ERR_ASSERT:
        return "InternalError";
    case ERR_ARITHMETIC:
        return "ArithmeticError";
    default:
        return "Error";
    }
}

// Helper function to get a specific line from source code
static const char* get_source_line(const char* source, int line_number, size_t* line_length) {
    if (!source || line_number <= 0) {
        *line_length = 0;
        return NULL;
    }

    const char* current = source;
    int current_line = 1;
    const char* line_start = source;

    // Find the start of the target line
    while (*current && current_line < line_number) {
        if (*current == '\n') {
            current_line++;
            line_start = current + 1;
        }
        current++;
    }

    if (current_line != line_number) {
        *line_length = 0;
        return NULL;
    }

    // Find the end of the line
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }

    *line_length = line_end - line_start;
    return line_start;
}

// Print error with caret pointing to the problem
static void print_error_with_caret(FILE* out, const SlateError* e, debug_location* debug_loc) {
    // Print error kind and message
    fprintf(out, "%s: %s\n", error_kind_name(e->kind), e->message);

    // If we have debug location with source text, show it
    if (debug_loc && debug_loc->source_text && e->line > 0 && e->column > 0) {
        fprintf(out, "    at line %d, column %d:\n", e->line, e->column);
        fprintf(out, "    %s\n", debug_loc->source_text);

        // Print caret pointing to the column
        fprintf(out, "    ");
        int caret_pos = e->column > 0 ? e->column - 1 : 0;
        for (int i = 0; i < caret_pos; i++) {
            fprintf(out, " ");
        }
        fprintf(out, "^\n");
    } else if (e->line > 0 && e->column > 0) {
        // Just show the location without source
        fprintf(out, "    at line %d, column %d\n", e->line, e->column);
    }
}

// Runtime error handling - uses context-aware error system
void runtime_error(vm_t* vm, const char* message, ...) {
    va_list args;
    va_start(args, message);

    char formatted_message[256];
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    if (vm) {
        // Use the new context-aware error system
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "%s", formatted_message);
    } else {
        // Fallback for cases without VM
        fprintf(stderr, "Runtime error: %s\n", formatted_message);
        exit(1);
    }
}

// Main runtime error function
void slate_runtime_error(vm_t* vm, ErrorKind kind, const char* file, int line, int column, const char* fmt, ...) {
    // Populate error structure
    vm->error.kind = kind;
    vm->error.file = file;
    vm->error.line = line;
    vm->error.column = column;

    // Format the error message
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(vm->error.message, sizeof(vm->error.message), fmt, ap);
    va_end(ap);

    // Get debug location if available
    debug_location* debug_loc = vm->current_debug;

    // Handle based on context
    switch (vm->context) {
    case CTX_INTERACTIVE:
        // Print error with caret and continue REPL
        print_error_with_caret(stderr, &vm->error, debug_loc);
        longjmp(vm->trap, 1);
        break;

    case CTX_SCRIPT:
        // Print error with caret and exit
        print_error_with_caret(stderr, &vm->error, debug_loc);
        exit(1);
        break;

    case CTX_TEST:
        // Silent - just longjmp back to test
        longjmp(vm->trap, 1);
        break;
    }
}

// Utility function that extracts debug location from values
void slate_runtime_error_with_debug(vm_t* vm, ErrorKind kind, value_t* a, value_t* b, const char* fmt, ...) {
    // Extract debug location - prefer b, then a, then vm->current_debug
    debug_location* debug_to_use = NULL;
    if (b && b->debug) {
        debug_to_use = b->debug;
    } else if (a && a->debug) {
        debug_to_use = a->debug;
    } else {
        debug_to_use = vm->current_debug;
    }

    // Format the error message
    char message[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof(message), fmt, ap);
    va_end(ap);

    // Call the main error function with extracted debug info
    slate_runtime_error(vm, kind,
                        NULL, // file will be extracted from debug location
                        debug_to_use ? debug_to_use->line : -1, debug_to_use ? debug_to_use->column : -1, "%s",
                        message);
}

// Wrapper function to handle library assert failures
void slate_library_assert_failed(const char* condition, const char* file, int line) {
    if (g_current_vm) {
        slate_runtime_error(g_current_vm, ERR_ASSERT, file, line, -1, "Library assertion failed: %s", condition);
    } else {
        fprintf(stderr, "Library assertion failed: %s\n", condition);
        abort();
    }
}
