#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Debug info functions
debug_info* debug_info_create(const char* source_code) {
    debug_info* debug = malloc(sizeof(debug_info));
    if (!debug) return NULL;
    
    debug->entries = NULL;
    debug->count = 0;
    debug->capacity = 0;
    debug->source_code = source_code; // Store reference (not owned)
    
    return debug;
}

void debug_info_destroy(debug_info* debug) {
    if (!debug) return;
    
    free(debug->entries);
    free(debug);
}

void debug_info_add_entry(debug_info* debug, size_t bytecode_offset, int line, int column) {
    if (!debug) return;
    
    // Grow array if needed
    if (debug->count >= debug->capacity) {
        size_t new_capacity = debug->capacity == 0 ? 8 : debug->capacity * 2;
        debug_info_entry* new_entries = realloc(debug->entries, sizeof(debug_info_entry) * new_capacity);
        if (!new_entries) return; // Out of memory
        
        debug->entries = new_entries;
        debug->capacity = new_capacity;
    }
    
    debug->entries[debug->count].bytecode_offset = bytecode_offset;
    debug->entries[debug->count].line = line;
    debug->entries[debug->count].column = column;
    debug->count++;
}