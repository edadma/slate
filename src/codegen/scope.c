#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Scope management functions
void codegen_init_scope_manager(codegen_t* codegen) {
    codegen->scope.locals = NULL;
    codegen->scope.local_count = 0;
    codegen->scope.local_capacity = 0;
    codegen->scope.scope_depth = 0; // Global scope
    codegen->scope.upvalues = NULL;
    codegen->scope.upvalue_count = 0;
    codegen->scope.upvalue_capacity = 0;
}

void codegen_cleanup_scope_manager(codegen_t* codegen) {
    // Free local variable names
    for (int i = 0; i < codegen->scope.local_count; i++) {
        free(codegen->scope.locals[i].name);
    }
    free(codegen->scope.locals);
    
    // Free upvalue names
    for (int i = 0; i < codegen->scope.upvalue_count; i++) {
        free(codegen->scope.upvalues[i].name);
    }
    free(codegen->scope.upvalues);
}

void codegen_begin_scope(codegen_t* codegen) {
    codegen->scope.scope_depth++;
}

void codegen_end_scope(codegen_t* codegen) {
    if (codegen->scope.scope_depth == 0) {
        codegen_error(codegen, "Cannot end global scope");
        return;
    }
    
    // Count how many locals need to be popped
    int locals_to_pop = 0;
    for (int i = codegen->scope.local_count - 1; i >= 0; i--) {
        if (codegen->scope.locals[i].depth < codegen->scope.scope_depth) {
            break; // Found variable from outer scope
        }
        locals_to_pop++;
    }
    
    // Emit OP_POP_N to clean up local variables with proper memory release
    if (locals_to_pop > 0) {
        codegen_emit_op(codegen, OP_POP_N);
        chunk_write_byte(codegen->chunk, (uint8_t)locals_to_pop);
        codegen->scope.local_count -= locals_to_pop;
    }
    
    codegen->scope.scope_depth--;
}

int codegen_declare_variable(codegen_t* codegen, const char* name, int is_immutable) {
    // Check if variable already exists in current scope
    for (int i = codegen->scope.local_count - 1; i >= 0; i--) {
        local_var_t* local = &codegen->scope.locals[i];
        if (local->depth < codegen->scope.scope_depth) {
            break; // Found variable from outer scope, no conflict
        }
        if (strcmp(local->name, name) == 0) {
            // Variable already exists - return its slot for re-initialization
            return local->slot;
        }
    }
    
    // Grow locals array if needed
    if (codegen->scope.local_count >= codegen->scope.local_capacity) {
        int new_capacity = codegen->scope.local_capacity == 0 ? 8 : codegen->scope.local_capacity * 2;
        local_var_t* new_locals = realloc(codegen->scope.locals, 
                                          new_capacity * sizeof(local_var_t));
        if (!new_locals) {
            codegen_error(codegen, "Out of memory for local variables");
            return -1;
        }
        codegen->scope.locals = new_locals;
        codegen->scope.local_capacity = new_capacity;
    }
    
    // Add new local variable
    local_var_t* local = &codegen->scope.locals[codegen->scope.local_count];
    local->name = strdup(name); // Make a copy of the name
    local->depth = codegen->scope.scope_depth;
    local->slot = codegen->scope.local_count; // Stack slot index relative to frame->slots
    local->is_initialized = 0; // Will be set to 1 after initialization
    local->is_immutable = is_immutable; // Store immutability flag
    
    return codegen->scope.local_count++;
}

int codegen_resolve_variable(codegen_t* codegen, const char* name, int* is_local, int* upvalue_index) {
    // Search for local variables (from innermost to outermost scope)
    for (int i = codegen->scope.local_count - 1; i >= 0; i--) {
        if (strcmp(codegen->scope.locals[i].name, name) == 0) {
            *is_local = 1;
            *upvalue_index = -1; // Not an upvalue
            return codegen->scope.locals[i].slot;
        }
    }
    
    // Search for upvalues
    int upvalue_slot = codegen_resolve_upvalue(codegen, name);
    if (upvalue_slot != -1) {
        *is_local = 0;
        *upvalue_index = upvalue_slot;
        return -1; // Upvalues are handled with special opcodes
    }
    
    // Not found in local scope or upvalues, treat as global
    *is_local = 0;
    *upvalue_index = -1;
    return -1; // Globals are handled differently
}

// Upvalue management functions
int codegen_add_upvalue(codegen_t* codegen, const char* name, int index, int is_local) {
    // Check if upvalue already exists
    for (int i = 0; i < codegen->scope.upvalue_count; i++) {
        if (strcmp(codegen->scope.upvalues[i].name, name) == 0) {
            return i; // Return existing upvalue index
        }
    }
    
    // Grow upvalues array if needed
    if (codegen->scope.upvalue_count >= codegen->scope.upvalue_capacity) {
        int new_capacity = codegen->scope.upvalue_capacity == 0 ? 8 : codegen->scope.upvalue_capacity * 2;
        upvalue_t* new_upvalues = realloc(codegen->scope.upvalues, 
                                         new_capacity * sizeof(upvalue_t));
        if (!new_upvalues) {
            codegen_error(codegen, "Out of memory for upvalues");
            return -1;
        }
        codegen->scope.upvalues = new_upvalues;
        codegen->scope.upvalue_capacity = new_capacity;
    }
    
    // Add new upvalue
    upvalue_t* upvalue = &codegen->scope.upvalues[codegen->scope.upvalue_count];
    upvalue->name = strdup(name);
    upvalue->index = index;
    upvalue->is_local = is_local;
    
    return codegen->scope.upvalue_count++;
}

int codegen_resolve_upvalue(codegen_t* codegen, const char* name) {
    // If no parent context, no upvalues can exist
    if (!codegen->parent) {
        return -1;
    }
    
    // Try to resolve in parent's local variables
    int is_local;
    int parent_upvalue_index;
    int parent_slot = codegen_resolve_variable(codegen->parent, name, &is_local, &parent_upvalue_index);
    
    if (is_local && parent_slot != -1) {
        // Found in parent's locals - capture it as local upvalue
        return codegen_add_upvalue(codegen, name, parent_slot, 1);
    }
    
    if (parent_upvalue_index != -1) {
        // Found in parent's upvalues - capture it as upvalue upvalue
        return codegen_add_upvalue(codegen, name, parent_upvalue_index, 0);
    }
    
    // Not found in parent scope
    return -1;
}