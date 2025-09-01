#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations
void codegen_emit_template_literal(codegen_t* codegen, ast_template_literal* node);

// Function compilation helper
function_t* codegen_compile_function(codegen_t* parent_codegen, ast_function* func_node) {
    if (!parent_codegen || !func_node) return NULL;
    
    // Create a new codegen context for the function
    codegen_t* func_codegen = codegen_create(parent_codegen->vm);
    if (!func_codegen) return NULL;
    
    // Create function object
    function_t* function = function_create(NULL);
    if (!function) {
        codegen_destroy(func_codegen);
        return NULL;
    }
    
    // Set up function metadata
    function->parameter_count = func_node->param_count;
    function->local_count = func_node->param_count; // Will be updated as locals are added
    
    // Copy parameter names (function_destroy expects to own them)
    if (func_node->param_count > 0) {
        function->parameter_names = malloc(sizeof(char*) * func_node->param_count);
        if (!function->parameter_names) {
            function_destroy(function);
            codegen_destroy(func_codegen);
            return NULL;
        }
        for (size_t i = 0; i < func_node->param_count; i++) {
            function->parameter_names[i] = malloc(strlen(func_node->parameters[i]) + 1);
            if (!function->parameter_names[i]) {
                // Clean up partially allocated names
                for (size_t j = 0; j < i; j++) {
                    free(function->parameter_names[j]);
                }
                free(function->parameter_names);
                function->parameter_names = NULL;
                function_destroy(function);
                codegen_destroy(func_codegen);
                return NULL;
            }
            strcpy(function->parameter_names[i], func_node->parameters[i]);
        }
    } else {
        function->parameter_names = NULL;
    }
    
    // Begin function scope and add parameters as local variables
    codegen_begin_scope(func_codegen);
    for (size_t i = 0; i < func_node->param_count; i++) {
        int slot = codegen_declare_variable(func_codegen, func_node->parameters[i]);
        if (slot == -1) {
            codegen_error(func_codegen, "Failed to declare parameter");
            function_destroy(function);
            codegen_destroy(func_codegen);
            return NULL;
        }
        // Parameters are always initialized
        func_codegen->scope.locals[slot].is_initialized = 1;
    }
    
    // Compile function body
    if (func_node->is_expression) {
        // Single expression function: compile expression and return result
        codegen_emit_expression(func_codegen, func_node->body);
        codegen_emit_op(func_codegen, OP_RETURN);
    } else {
        // Block function: compile statements
        ast_block* block = (ast_block*)func_node->body;
        for (size_t i = 0; i < block->statement_count; i++) {
            codegen_emit_statement(func_codegen, block->statements[i]);
            if (func_codegen->had_error) break;
        }
        
        // If no explicit return, return null
        if (!func_codegen->had_error) {
            codegen_emit_op_operand(func_codegen, OP_PUSH_CONSTANT, 
                                  chunk_add_constant(func_codegen->chunk, make_null()));
            codegen_emit_op(func_codegen, OP_RETURN);
        }
    }
    
    // Check for compilation errors
    if (func_codegen->had_error) {
        function_destroy(function);
        codegen_destroy(func_codegen);
        return NULL;
    }
    
    // Transfer bytecode and constants to function
    function->bytecode_length = func_codegen->chunk->count;
    function->bytecode = malloc(function->bytecode_length);
    if (!function->bytecode) {
        function_destroy(function);
        codegen_destroy(func_codegen);
        return NULL;
    }
    memcpy(function->bytecode, func_codegen->chunk->code, function->bytecode_length);
    
    // Transfer constants
    function->constant_count = func_codegen->chunk->constant_count;
    if (function->constant_count > 0) {
        function->constants = malloc(sizeof(value_t) * function->constant_count);
        if (!function->constants) {
            function_destroy(function);
            codegen_destroy(func_codegen);
            return NULL;
        }
        // Copy and retain each constant value
        for (size_t i = 0; i < function->constant_count; i++) {
            function->constants[i] = func_codegen->chunk->constants[i];
            // Retain reference-counted values (strings, arrays, objects, etc.)
            vm_retain(function->constants[i]);
        }
    }
    
    // Update local count
    function->local_count = func_codegen->scope.local_count;
    
    // Clean up function codegen context
    codegen_destroy(func_codegen);
    
    return function;
}

// Function expression emission (creates closure on stack)
void codegen_emit_function(codegen_t* codegen, ast_function* node) {
    // Compile the function using the helper
    function_t* function = codegen_compile_function(codegen, node);
    if (!function) {
        codegen_error(codegen, "Failed to compile function");
        return;
    }
    
    // Store function in function table and create closure
    size_t func_index = vm_add_function(codegen->vm, function);
    size_t constant = chunk_add_constant(codegen->chunk, make_int32((int32_t)func_index));
    codegen_emit_op_operand(codegen, OP_CLOSURE, (uint16_t)constant);
}