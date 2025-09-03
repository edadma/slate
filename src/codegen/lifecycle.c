#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Code generator functions
codegen_t* codegen_create(vm_t* vm) {
    codegen_t* codegen = malloc(sizeof(codegen_t));
    if (!codegen) return NULL;
    
    codegen->chunk = chunk_create();
    codegen->vm = vm; // Store VM reference for function table access
    codegen->parent = NULL; // No parent by default
    codegen->had_error = 0;
    codegen->debug_mode = 0; // No debug info by default
    codegen->loop_contexts = NULL;
    codegen->loop_depth = 0;
    codegen->loop_capacity = 0;
    
    // Initialize scope manager
    codegen_init_scope_manager(codegen);
    
    return codegen;
}

codegen_t* codegen_create_with_debug(vm_t* vm, const char* source_code) {
    codegen_t* codegen = malloc(sizeof(codegen_t));
    if (!codegen) return NULL;
    
    codegen->chunk = chunk_create_with_debug(source_code);
    if (!codegen->chunk) {
        free(codegen);
        return NULL;
    }
    
    codegen->vm = vm; // Store VM reference for function table access
    codegen->parent = NULL; // No parent by default
    codegen->had_error = 0;
    codegen->debug_mode = 1; // Enable debug info
    codegen->loop_contexts = NULL;
    codegen->loop_depth = 0;
    codegen->loop_capacity = 0;
    
    // Initialize scope manager
    codegen_init_scope_manager(codegen);
    
    return codegen;
}

void codegen_destroy(codegen_t* codegen) {
    if (!codegen) return;
    
    chunk_destroy(codegen->chunk);
    
    // Clean up loop contexts
    for (size_t i = 0; i < codegen->loop_depth; i++) {
        free(codegen->loop_contexts[i].break_jumps);
    }
    free(codegen->loop_contexts);
    
    // Clean up scope manager
    codegen_cleanup_scope_manager(codegen);
    
    free(codegen);
}