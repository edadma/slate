#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Main compilation function
function_t* codegen_compile(codegen_t* codegen, ast_program* program) {
    if (!codegen || !program) return NULL;
    
    // Generate code for all statements
    for (size_t i = 0; i < program->statement_count; i++) {
        codegen_emit_statement(codegen, program->statements[i]);
        if (codegen->had_error) return NULL;
    }
    
    // Emit halt instruction
    codegen_emit_op(codegen, OP_HALT);
    
    // Create function from chunk
    function_t* function = function_create("main");
    if (!function) return NULL;
    
    // Transfer bytecode (deep copy)
    function->bytecode_length = codegen->chunk->count;
    function->bytecode = malloc(function->bytecode_length);
    if (!function->bytecode) {
        function_destroy(function);
        return NULL;
    }
    memcpy(function->bytecode, codegen->chunk->code, function->bytecode_length);
    
    // Transfer constants (deep copy with proper retention to avoid sharing)
    function->constant_count = codegen->chunk->constant_count;
    if (function->constant_count > 0) {
        function->constants = malloc(sizeof(value_t) * function->constant_count);
        if (!function->constants) {
            function_destroy(function);
            return NULL;
        }
        // Copy and retain each constant value
        for (size_t i = 0; i < function->constant_count; i++) {
            function->constants[i] = codegen->chunk->constants[i];
            // Retain reference-counted values (strings, arrays, objects, etc.)
            vm_retain(function->constants[i]);
        }
    } else {
        function->constants = NULL;
    }
    
    // Transfer debug info
    function->debug = codegen->chunk->debug;
    codegen->chunk->debug = NULL; // Transfer ownership
    
    return function;
}