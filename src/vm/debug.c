#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"
#include "runtime_error.h"
#include "vm.h"

// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text) {
    debug_location* debug = malloc(sizeof(debug_location));
    if (!debug)
        return NULL;

    debug->line = line;
    debug->column = column;
    debug->source_text = source_text; // Not owned, just a reference
    return debug;
}

debug_location* debug_location_copy(const debug_location* debug) {
    if (!debug)
        return NULL;

    return debug_location_create(debug->line, debug->column, debug->source_text);
}

void debug_location_free(debug_location* debug) {
    if (debug && (uintptr_t)debug > 0x1000) { // Basic sanity check - valid pointers are usually much higher
        free(debug);
    }
}

// Debug utilities

// Helper function to get a specific line from source code (from parser.c)
static const char* get_source_line(const char* source, int line_number, size_t* line_length) {
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

// Find debug info entry for a given bytecode offset
void* vm_get_debug_info_at(function_t* function, size_t bytecode_offset) {
    if (!function || !function->debug) {
        return NULL;
    }

    debug_info* debug = (debug_info*)function->debug;

    // Find the closest debug info entry <= bytecode_offset
    debug_info_entry* best_entry = NULL;
    for (size_t i = 0; i < debug->count; i++) {
        if (debug->entries[i].bytecode_offset <= bytecode_offset) {
            best_entry = &debug->entries[i];
        } else {
            break; // Entries should be in order
        }
    }

    return best_entry ? debug : NULL;
}

// Enhanced error reporting using value debug information
void vm_runtime_error_with_values(vm_t* vm, const char* format, const value_t* a, const value_t* b,
                                  debug_location* location) {
    // Format the error message with value types
    char formatted_message[256];
    snprintf(formatted_message, sizeof(formatted_message), format, value_type_name(a->type),
             b ? value_type_name(b->type) : "");

    // Use the best debug location available (preference order: location param, a->debug, b->debug, current_debug)
    debug_location* debug_to_use = location;
    if (!debug_to_use && a)
        debug_to_use = a->debug;
    if (!debug_to_use && b)
        debug_to_use = b->debug;
    if (!debug_to_use)
        debug_to_use = vm->current_debug;

    // Extract line and column from debug location for the new error system
    int line = debug_to_use ? debug_to_use->line : -1;
    int column = debug_to_use ? debug_to_use->column : -1;

    // Use the new context-aware error system
    slate_runtime_error(vm, ERR_TYPE, __FILE__, line, column, "%s", formatted_message);
}

// Helper function to get value type name for error messages
const char* value_type_name(value_type type) {
    switch (type) {
    case VAL_NULL:
        return "null";
    case VAL_UNDEFINED:
        return "undefined";
    case VAL_BOOLEAN:
        return "boolean";
    case VAL_INT32:
        return "int32";
    case VAL_BIGINT:
        return "bigint";
    case VAL_FLOAT32:
        return "float32";
    case VAL_FLOAT64:
        return "float64";
    case VAL_STRING:
        return "string";
    case VAL_STRING_BUILDER:
        return "string_builder";
    case VAL_ARRAY:
        return "array";
    case VAL_OBJECT:
        return "object";
    case VAL_CLASS:
        return "class";
    case VAL_RANGE:
        return "range";
    case VAL_ITERATOR:
        return "iterator";
    case VAL_BUFFER:
        return "buffer";
    case VAL_BUFFER_BUILDER:
        return "buffer_builder";
    case VAL_BUFFER_READER:
        return "buffer_reader";
    case VAL_FUNCTION:
        return "function";
    case VAL_CLOSURE:
        return "closure";
    case VAL_NATIVE:
        return "builtin";
    case VAL_BOUND_METHOD:
        return "bound_method";
    case VAL_LOCAL_DATE:
        return "LocalDate";
    case VAL_LOCAL_TIME:
        return "LocalTime";
    case VAL_LOCAL_DATETIME:
        return "LocalDateTime";
    case VAL_ZONE:
        return "Zone";
    case VAL_DATE:
        return "Date";
    case VAL_INSTANT:
        return "Instant";
    case VAL_DURATION:
        return "Duration";
    case VAL_PERIOD:
        return "Period";
    default:
        return "unknown";
    }
}
