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

// Bytecode chunk functions
bytecode_chunk* chunk_create(void) {
    bytecode_chunk* chunk = malloc(sizeof(bytecode_chunk));
    if (!chunk) return NULL;
    
    chunk->code = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->constants = NULL;
    chunk->constant_count = 0;
    chunk->constant_capacity = 0;
    chunk->debug = NULL; // No debug info by default
    
    return chunk;
}

bytecode_chunk* chunk_create_with_debug(const char* source_code) {
    bytecode_chunk* chunk = chunk_create();
    if (!chunk) return NULL;
    
    chunk->debug = debug_info_create(source_code);
    if (!chunk->debug) {
        chunk_destroy(chunk);
        return NULL;
    }
    
    return chunk;
}

void chunk_destroy(bytecode_chunk* chunk) {
    if (!chunk) return;
    
    free(chunk->code);
    
    for (size_t i = 0; i < chunk->constant_count; i++) {
        free_value(chunk->constants[i]);
    }
    free(chunk->constants);
    
    debug_info_destroy(chunk->debug);
    
    free(chunk);
}

void chunk_write_byte(bytecode_chunk* chunk, uint8_t byte) {
    if (chunk->count >= chunk->capacity) {
        size_t new_capacity = chunk->capacity == 0 ? 8 : chunk->capacity * 2;
        chunk->code = realloc(chunk->code, new_capacity);
        chunk->capacity = new_capacity;
    }
    
    chunk->code[chunk->count++] = byte;
}

void chunk_write_opcode(bytecode_chunk* chunk, opcode op) {
    chunk_write_byte(chunk, (uint8_t)op);
}

void chunk_write_operand(bytecode_chunk* chunk, uint16_t operand) {
    chunk_write_byte(chunk, (uint8_t)(operand & 0xFF));
    chunk_write_byte(chunk, (uint8_t)((operand >> 8) & 0xFF));
}

size_t chunk_add_constant(bytecode_chunk* chunk, value_t value) {
    if (chunk->constant_count >= chunk->constant_capacity) {
        size_t new_capacity = chunk->constant_capacity == 0 ? 8 : chunk->constant_capacity * 2;
        chunk->constants = realloc(chunk->constants, sizeof(value_t) * new_capacity);
        chunk->constant_capacity = new_capacity;
    }
    
    chunk->constants[chunk->constant_count] = value;
    return chunk->constant_count++;
}

void chunk_add_debug_info(bytecode_chunk* chunk, int line, int column) {
    if (chunk->debug) {
        debug_info_add_entry(chunk->debug, chunk->count, line, column);
    }
}

// Code generator functions
codegen_t* codegen_create(slate_vm* vm) {
    codegen_t* codegen = malloc(sizeof(codegen_t));
    if (!codegen) return NULL;
    
    codegen->chunk = chunk_create();
    codegen->vm = vm; // Store VM reference for function table access
    codegen->had_error = 0;
    codegen->debug_mode = 0; // No debug info by default
    codegen->loop_contexts = NULL;
    codegen->loop_depth = 0;
    codegen->loop_capacity = 0;
    
    return codegen;
}

codegen_t* codegen_create_with_debug(slate_vm* vm, const char* source_code) {
    codegen_t* codegen = malloc(sizeof(codegen_t));
    if (!codegen) return NULL;
    
    codegen->chunk = chunk_create_with_debug(source_code);
    if (!codegen->chunk) {
        free(codegen);
        return NULL;
    }
    
    codegen->vm = vm; // Store VM reference for function table access
    codegen->had_error = 0;
    codegen->debug_mode = 1; // Enable debug info
    codegen->loop_contexts = NULL;
    codegen->loop_depth = 0;
    codegen->loop_capacity = 0;
    
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
    
    free(codegen);
}

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
    
    // Transfer ownership of bytecode and constants
    function->bytecode = codegen->chunk->code;
    function->bytecode_length = codegen->chunk->count;
    function->constants = codegen->chunk->constants;
    function->constant_count = codegen->chunk->constant_count;
    function->debug = codegen->chunk->debug; // Transfer debug info
    
    // Clear chunk so it won't be freed
    codegen->chunk->code = NULL;
    codegen->chunk->constants = NULL;
    codegen->chunk->debug = NULL; // Transfer ownership of debug info
    codegen->chunk->count = 0;
    codegen->chunk->constant_count = 0;
    
    return function;
}

// Expression code generation
void codegen_emit_expression(codegen_t* codegen, ast_node* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_INTEGER:
            codegen_emit_integer(codegen, (ast_integer*)expr);
            break;
            
        case AST_BIGINT:
            codegen_emit_bigint(codegen, (ast_bigint*)expr);
            break;
            
        case AST_NUMBER:
            codegen_emit_number(codegen, (ast_number*)expr);
            break;
            
        case AST_STRING:
            codegen_emit_string(codegen, (ast_string*)expr);
            break;
            
        case AST_BOOLEAN:
            codegen_emit_boolean(codegen, (ast_boolean*)expr);
            break;
            
        case AST_NULL:
            codegen_emit_null(codegen, (ast_null*)expr);
            break;
            
        case AST_UNDEFINED:
            codegen_emit_undefined(codegen, (ast_undefined*)expr);
            break;
            
        case AST_IDENTIFIER:
            codegen_emit_identifier(codegen, (ast_identifier*)expr);
            break;
            
        case AST_BINARY_OP:
            codegen_emit_binary_op(codegen, (ast_binary_op*)expr);
            break;
            
        case AST_RANGE:
            codegen_emit_range(codegen, (ast_range*)expr);
            break;
            
        case AST_UNARY_OP:
            codegen_emit_unary_op(codegen, (ast_unary_op*)expr);
            break;
            
        case AST_ARRAY:
            codegen_emit_array(codegen, (ast_array*)expr);
            break;
            
        case AST_OBJECT_LITERAL:
            codegen_emit_object(codegen, (ast_object_literal*)expr);
            break;
            
        case AST_INDEX: {
            ast_index* index_node = (ast_index*)expr;
            // Generate object expression
            codegen_emit_expression(codegen, index_node->object);
            // Generate index expression
            codegen_emit_expression(codegen, index_node->index);
            // Emit index operation
            codegen_emit_op(codegen, OP_GET_INDEX);
            break;
        }
        
        case AST_MEMBER: {
            ast_member* member_node = (ast_member*)expr;
            // Generate object expression
            codegen_emit_expression(codegen, member_node->object);
            // Generate property name as string constant
            size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member_node->property));
            codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);
            // Emit property access operation
            codegen_emit_op(codegen, OP_GET_PROPERTY);
            break;
        }
        
        case AST_CALL: {
            ast_call* call_node = (ast_call*)expr;
            // Generate function/callable expression first
            codegen_emit_expression(codegen, call_node->function);
            // Generate arguments (pushed left to right)
            for (size_t i = 0; i < call_node->arg_count; i++) {
                codegen_emit_expression(codegen, call_node->arguments[i]);
            }
            // Emit call operation with argument count
            codegen_emit_op_operand(codegen, OP_CALL, (uint16_t)call_node->arg_count);
            break;
        }
        
        case AST_FUNCTION: {
            ast_function* func_node = (ast_function*)expr;
            
            // Create function object
            function_t* function = function_create(NULL);
            function->parameter_count = func_node->param_count;
            function->parameter_names = func_node->parameters;
            function->local_count = func_node->param_count;
            
            // Use db_builder for guaranteed safe bytecode construction
            db_builder builder = db_builder_new(64);
            if (!builder) {
                codegen_error(codegen, "Failed to create bytecode builder");
                function_destroy(function);
                break;
            }
            
            // Build minimal function: PUSH_CONSTANT 42, RETURN
            // Create constants array
            function->constants = malloc(sizeof(value_t));
            function->constants[0] = make_int32(42);
            function->constant_count = 1;
            
            // Build bytecode: opcode + 16-bit operand + return
            uint8_t bytecode[] = {
                OP_PUSH_CONSTANT,
                0, 0,  // 16-bit constant index 0 (little-endian)
                OP_RETURN
            };
            
            db_builder_append(builder, bytecode, sizeof(bytecode));
            
            // Transfer bytecode to function
            db_buffer bytecode_buf = db_builder_finish(&builder);
            function->bytecode_length = db_size(bytecode_buf);
            function->bytecode = malloc(function->bytecode_length);
            memcpy(function->bytecode, bytecode_buf, function->bytecode_length);
            
            // Clean up
            db_release(&bytecode_buf);
            
            // Store function in function table and create closure
            size_t func_index = vm_add_function(codegen->vm, function);
            size_t constant = chunk_add_constant(codegen->chunk, make_int32((int32_t)func_index));
            codegen_emit_op_operand(codegen, OP_CLOSURE, (uint16_t)constant);
            break;
        }
        
        case AST_ASSIGNMENT: {
            ast_assignment* assign = (ast_assignment*)expr;
            
            // Generate code for the value to assign
            codegen_emit_expression(codegen, assign->value);
            
            // For expressions, we need to duplicate the value so it remains on stack
            codegen_emit_op(codegen, OP_DUP);
            
            // Handle assignment target
            if (assign->target->type == AST_IDENTIFIER) {
                ast_identifier* var = (ast_identifier*)assign->target;
                size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
                
                // Add debug info for the assignment operation
                chunk_add_debug_info(codegen->chunk, assign->base.line, assign->base.column);
                codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
            } else {
                codegen_error(codegen, "Only variable assignments are currently supported");
            }
            break;
        }
        
        case AST_COMPOUND_ASSIGNMENT: {
            ast_compound_assignment* comp_assign = (ast_compound_assignment*)expr;
            
            // For compound assignments in expressions, we emit the same logic as statements
            codegen_emit_compound_assignment(codegen, comp_assign);
            
            break;
        }

        case AST_IF:
            codegen_emit_if(codegen, (ast_if*)expr);
            break;

        case AST_BLOCK:
            codegen_emit_block_expression(codegen, (ast_block*)expr);
            break;
            
        case AST_BREAK:
            codegen_emit_break(codegen, (ast_break*)expr);
            break;
            
        case AST_CONTINUE:
            codegen_emit_continue(codegen, (ast_continue*)expr);
            break;
            
        default:
            codegen_error(codegen, "Unknown expression type");
            break;
    }
}

// Statement code generation
void codegen_emit_statement(codegen_t* codegen, ast_node* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_VAR_DECLARATION:
            codegen_emit_var_declaration(codegen, (ast_var_declaration*)stmt);
            break;
            
        case AST_EXPRESSION_STMT:
            codegen_emit_expression_stmt(codegen, (ast_expression_stmt*)stmt);
            break;
            
        case AST_BLOCK:
            codegen_emit_block(codegen, (ast_block*)stmt);
            break;
            
        case AST_IF:
            codegen_emit_if(codegen, (ast_if*)stmt);
            break;
            
        case AST_WHILE:
            codegen_emit_while(codegen, (ast_while*)stmt);
            break;
            
        case AST_LOOP:
            codegen_emit_infinite_loop(codegen, (ast_loop*)stmt);
            break;
            
        case AST_BREAK:
            codegen_emit_break(codegen, (ast_break*)stmt);
            break;
            
        case AST_CONTINUE:
            codegen_emit_continue(codegen, (ast_continue*)stmt);
            break;
            
        case AST_RETURN:
            codegen_emit_return(codegen, (ast_return*)stmt);
            break;
            
        case AST_ASSIGNMENT:
            codegen_emit_assignment(codegen, (ast_assignment*)stmt);
            break;
            
        case AST_COMPOUND_ASSIGNMENT:
            codegen_emit_compound_assignment(codegen, (ast_compound_assignment*)stmt);
            break;
            
        default:
            codegen_error(codegen, "Unknown statement type");
            break;
    }
}

// Literal emission functions
void codegen_emit_integer(codegen_t* codegen, ast_integer* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    size_t constant = chunk_add_constant(codegen->chunk, make_int32(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_bigint(codegen_t* codegen, ast_bigint* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    // Retain the BigInt value when transferring from AST to VM
    di_int retained_value = di_retain(node->value);
    size_t constant = chunk_add_constant(codegen->chunk, make_bigint(retained_value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_number(codegen_t* codegen, ast_number* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    size_t constant = chunk_add_constant(codegen->chunk, make_number(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_string(codegen_t* codegen, ast_string* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_boolean(codegen_t* codegen, ast_boolean* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    if (node->value) {
        codegen_emit_op(codegen, OP_PUSH_TRUE);
    } else {
        codegen_emit_op(codegen, OP_PUSH_FALSE);
    }
}

void codegen_emit_null(codegen_t* codegen, ast_null* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    codegen_emit_op(codegen, OP_PUSH_NULL);
}

void codegen_emit_undefined(codegen_t* codegen, ast_undefined* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
}

void codegen_emit_identifier(codegen_t* codegen, ast_identifier* node) {
    // For now, treat all identifiers as globals
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->name));
    codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
}

void codegen_emit_binary_op(codegen_t* codegen, ast_binary_op* node) {
    // Generate left operand
    codegen_emit_expression(codegen, node->left);
    
    // Generate right operand  
    codegen_emit_expression(codegen, node->right);
    
    // Generate operator without debug info (operands already have their debug info)
    switch (node->op) {
        case BIN_ADD:           codegen_emit_op(codegen, OP_ADD); break;
        case BIN_SUBTRACT:      codegen_emit_op(codegen, OP_SUBTRACT); break;
        case BIN_MULTIPLY:      codegen_emit_op(codegen, OP_MULTIPLY); break;
        case BIN_DIVIDE:        codegen_emit_op(codegen, OP_DIVIDE); break;
        case BIN_MOD:           codegen_emit_op(codegen, OP_MOD); break;
        case BIN_POWER:         codegen_emit_op(codegen, OP_POWER); break;
        case BIN_EQUAL:         codegen_emit_op(codegen, OP_EQUAL); break;
        case BIN_NOT_EQUAL:     codegen_emit_op(codegen, OP_NOT_EQUAL); break;
        case BIN_LESS:          codegen_emit_op(codegen, OP_LESS); break;
        case BIN_LESS_EQUAL:    codegen_emit_op(codegen, OP_LESS_EQUAL); break;
        case BIN_GREATER:       codegen_emit_op(codegen, OP_GREATER); break;
        case BIN_GREATER_EQUAL: codegen_emit_op(codegen, OP_GREATER_EQUAL); break;
        case BIN_LOGICAL_AND:   codegen_emit_op(codegen, OP_AND); break;
        case BIN_LOGICAL_OR:    codegen_emit_op(codegen, OP_OR); break;
        case BIN_BITWISE_AND:   codegen_emit_op(codegen, OP_BITWISE_AND); break;
        case BIN_BITWISE_OR:    codegen_emit_op(codegen, OP_BITWISE_OR); break;
        case BIN_BITWISE_XOR:   codegen_emit_op(codegen, OP_BITWISE_XOR); break;
        case BIN_LEFT_SHIFT:    codegen_emit_op(codegen, OP_LEFT_SHIFT); break;
        case BIN_RIGHT_SHIFT:   codegen_emit_op(codegen, OP_RIGHT_SHIFT); break;
        case BIN_LOGICAL_RIGHT_SHIFT: codegen_emit_op(codegen, OP_LOGICAL_RIGHT_SHIFT); break;
        case BIN_FLOOR_DIV:     codegen_emit_op(codegen, OP_FLOOR_DIV); break;
    }
}

void codegen_emit_range(codegen_t* codegen, ast_range* node) {
    // Emit debug location before building range
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    // Generate start value
    codegen_emit_expression(codegen, node->start);
    
    // Generate end value
    codegen_emit_expression(codegen, node->end);
    
    // Build range object (operand indicates whether it's exclusive)
    codegen_emit_op_operand(codegen, OP_BUILD_RANGE, (uint16_t)(node->exclusive ? 1 : 0));
}

// Helper function to check if an AST node represents an l-value
bool is_lvalue(ast_node* node) {
    if (!node) return false;
    
    switch (node->type) {
        case AST_IDENTIFIER:
            // Variables are l-values (when variables are implemented)
            return true;
        case AST_INDEX:
            // Array/object indexing is an l-value (e.g., arr[0]++)
            return true;
        case AST_MEMBER:
            // Object member access is an l-value (e.g., obj.prop++)
            return true;
        default:
            // Literals, expressions, etc. are not l-values
            return false;
    }
}

void codegen_emit_unary_op(codegen_t* codegen, ast_unary_op* node) {
    // Check if increment/decrement operators are applied to l-values
    if (node->op == UN_PRE_INCREMENT || node->op == UN_PRE_DECREMENT ||
        node->op == UN_POST_INCREMENT || node->op == UN_POST_DECREMENT) {
        if (!is_lvalue(node->operand)) {
            printf("Compile error: %s operator can only be applied to l-values (variables, array elements, object properties)\n",
                   (node->op == UN_PRE_INCREMENT || node->op == UN_POST_INCREMENT) ? "Increment" : "Decrement");
            codegen->had_error = 1;
            return;
        }
    }
    
    // Generate operand
    codegen_emit_expression(codegen, node->operand);
    
    // Generate operator
    switch (node->op) {
        case UN_NEGATE: codegen_emit_op(codegen, OP_NEGATE); break;
        case UN_NOT:    codegen_emit_op(codegen, OP_NOT); break;
        case UN_BITWISE_NOT: codegen_emit_op(codegen, OP_BITWISE_NOT); break;
        case UN_PRE_INCREMENT: codegen_emit_op(codegen, OP_INCREMENT); break;
        case UN_PRE_DECREMENT: codegen_emit_op(codegen, OP_DECREMENT); break;
        case UN_POST_INCREMENT:
            // For post-increment, we need to duplicate the value first
            codegen_emit_op(codegen, OP_DUP);
            codegen_emit_op(codegen, OP_INCREMENT);
            codegen_emit_op(codegen, OP_POP);  // Pop the incremented value, leaving original
            break;
        case UN_POST_DECREMENT:
            // For post-decrement, we need to duplicate the value first
            codegen_emit_op(codegen, OP_DUP);
            codegen_emit_op(codegen, OP_DECREMENT);
            codegen_emit_op(codegen, OP_POP);  // Pop the decremented value, leaving original
            break;
    }
}

void codegen_emit_array(codegen_t* codegen, ast_array* node) {
    // Generate all elements
    for (size_t i = 0; i < node->count; i++) {
        codegen_emit_expression(codegen, node->elements[i]);
    }
    
    // Build array from stack
    codegen_emit_op_operand(codegen, OP_BUILD_ARRAY, (uint16_t)node->count);
}

void codegen_emit_object(codegen_t* codegen, ast_object_literal* node) {
    // Generate key-value pairs
    for (size_t i = 0; i < node->property_count; i++) {
        // Push key
        size_t key_constant = chunk_add_constant(codegen->chunk, make_string(node->properties[i].key));
        codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)key_constant);
        
        // Push value
        codegen_emit_expression(codegen, node->properties[i].value);
    }
    
    // Build object from stack
    codegen_emit_op_operand(codegen, OP_BUILD_OBJECT, (uint16_t)node->property_count);
}

// Function codegen will be implemented later with proper design

// Statement emission functions
void codegen_emit_var_declaration(codegen_t* codegen, ast_var_declaration* node) {
    if (node->initializer) {
        codegen_emit_expression(codegen, node->initializer);
    } else {
        codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
    }
    
    // Duplicate the value so we can store it and also set result
    codegen_emit_op(codegen, OP_DUP);
    
    // Define global variable
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->name));
    codegen_emit_op_operand(codegen, OP_DEFINE_GLOBAL, (uint16_t)constant);
    
    // Set result register with the initialization value
    codegen_emit_op(codegen, OP_SET_RESULT);
}

void codegen_emit_assignment(codegen_t* codegen, ast_assignment* node) {
    // Generate code for the value to assign
    codegen_emit_expression(codegen, node->value);
    
    // For now, only handle simple variable assignments
    if (node->target->type == AST_IDENTIFIER) {
        ast_identifier* var = (ast_identifier*)node->target;
        codegen_emit_op(codegen, OP_DUP);
        size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
        
        // Add debug info for the assignment operation
        chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
        codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
    } else {
        codegen_error(codegen, "Only variable assignments are currently supported");
    }
}

void codegen_emit_compound_assignment(codegen_t* codegen, ast_compound_assignment* node) {
    // For now, only handle simple variable compound assignments
    if (node->target->type != AST_IDENTIFIER) {
        codegen_error(codegen, "Only variable compound assignments are currently supported");
        return;
    }
    
    ast_identifier* var = (ast_identifier*)node->target;
    size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
    
    // Get the current value of the variable
    chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
    codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
    
    // Generate code for the right-hand side value
    codegen_emit_expression(codegen, node->value);
    
    // Emit the appropriate binary operation
    switch (node->op) {
        case BIN_ADD:      codegen_emit_op(codegen, OP_ADD); break;
        case BIN_SUBTRACT: codegen_emit_op(codegen, OP_SUBTRACT); break;
        case BIN_MULTIPLY: codegen_emit_op(codegen, OP_MULTIPLY); break;
        case BIN_DIVIDE:   codegen_emit_op(codegen, OP_DIVIDE); break;
        case BIN_MOD:      codegen_emit_op(codegen, OP_MOD); break;
        case BIN_POWER:    codegen_emit_op(codegen, OP_POWER); break;
        case BIN_BITWISE_AND:  codegen_emit_op(codegen, OP_BITWISE_AND); break;
        case BIN_BITWISE_OR:   codegen_emit_op(codegen, OP_BITWISE_OR); break;
        case BIN_BITWISE_XOR:  codegen_emit_op(codegen, OP_BITWISE_XOR); break;
        case BIN_LOGICAL_AND:  codegen_emit_op(codegen, OP_AND); break;
        case BIN_LOGICAL_OR:   codegen_emit_op(codegen, OP_OR); break;
        default:
            codegen_error(codegen, "Unsupported compound assignment operation");
            return;
    }
    
    // Duplicate the result for expression contexts
    codegen_emit_op(codegen, OP_DUP);
    
    // Store the result back to the variable
    chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
    codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
}

void codegen_emit_expression_stmt(codegen_t* codegen, ast_expression_stmt* node) {
    codegen_emit_expression(codegen, node->expression);
    codegen_emit_op(codegen, OP_SET_RESULT); // Pop and store in result register
}

void codegen_emit_block(codegen_t* codegen, ast_block* node) {
    for (size_t i = 0; i < node->statement_count; i++) {
        codegen_emit_statement(codegen, node->statements[i]);
    }
}

void codegen_emit_if(codegen_t* codegen, ast_if* node) {
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false (condition remains on stack)
    size_t else_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    codegen_emit_op(codegen, OP_POP); // Pop condition in then branch
    
    // Generate then branch
    codegen_emit_expression(codegen, node->then_stmt);
    
    size_t end_jump = codegen_emit_jump(codegen, OP_JUMP);
    
    // Patch else jump
    codegen_patch_jump(codegen, else_jump);
    codegen_emit_op(codegen, OP_POP); // Pop condition in else branch
    
    // Generate else branch if present
    if (node->else_stmt) {
        codegen_emit_expression(codegen, node->else_stmt);
    } else {
        // If no else branch, push null as the result
        codegen_emit_op(codegen, OP_PUSH_NULL);
    }
    
    // Patch end jump
    codegen_patch_jump(codegen, end_jump);
}

void codegen_emit_block_expression(codegen_t* codegen, ast_block* node) {
    if (node->statement_count == 0) {
        // Empty block returns null
        codegen_emit_op(codegen, OP_PUSH_NULL);
        return;
    }
    
    // Execute all statements except the last one normally
    for (size_t i = 0; i < node->statement_count - 1; i++) {
        codegen_emit_statement(codegen, node->statements[i]);
    }
    
    // The last statement must be an expression statement (validated by parser)
    // Emit its expression directly to leave the value on the stack
    ast_node* last_stmt = node->statements[node->statement_count - 1];
    ast_expression_stmt* expr_stmt = (ast_expression_stmt*)last_stmt;
    codegen_emit_expression(codegen, expr_stmt->expression);
}

void codegen_emit_while(codegen_t* codegen, ast_while* node) {
    size_t loop_start = codegen->chunk->count;
    
    // Begin loop context for break and continue statements
    codegen_push_loop(codegen, loop_start);
    
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false (exit loop)
    size_t exit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    codegen_emit_op(codegen, OP_POP); // Pop condition
    
    // Generate body
    codegen_emit_statement(codegen, node->body);
    
    // Jump back to loop start using negative offset
    size_t current_pos = codegen->chunk->count;
    size_t backward_distance = current_pos - loop_start + 3; // +3 for the JUMP instruction itself
    if (backward_distance > UINT16_MAX) {
        codegen_error(codegen, "Loop body too large");
        return;
    }
    // Emit backward jump (negative offset)
    codegen_emit_op_operand(codegen, OP_JUMP, (uint16_t)(-backward_distance));
    
    // Patch exit jump
    codegen_patch_jump(codegen, exit_jump);
    codegen_emit_op(codegen, OP_POP); // Pop condition
    
    // End loop context and patch break statements
    codegen_pop_loop(codegen);
}

void codegen_emit_infinite_loop(codegen_t* codegen, ast_loop* node) {
    size_t loop_start = codegen->chunk->count;
    
    // Begin loop context for break and continue statements
    codegen_push_loop(codegen, loop_start);
    
    // Generate body (no condition needed for infinite loop)
    codegen_emit_statement(codegen, node->body);
    
    // Jump back to loop start using negative offset
    size_t current_pos = codegen->chunk->count;
    size_t backward_distance = current_pos - loop_start + 3; // +3 for the JUMP instruction itself
    if (backward_distance > UINT16_MAX) {
        codegen_error(codegen, "Loop body too large");
        return;
    }
    // Emit backward jump (negative offset) - this creates the infinite loop
    codegen_emit_op_operand(codegen, OP_JUMP, (uint16_t)(-backward_distance));
    
    // End loop context and patch break statements
    codegen_pop_loop(codegen);
}

void codegen_emit_break(codegen_t* codegen, ast_break* node) {
    loop_context_t* loop = codegen_current_loop(codegen);
    if (!loop) {
        codegen_error(codegen, "Break statement outside of loop");
        return;
    }
    
    // Emit a jump that will be patched later when the loop ends
    size_t jump_offset = codegen_emit_jump(codegen, OP_JUMP);
    
    // Add to the current loop's list of break jumps to patch
    if (loop->break_count >= loop->break_capacity) {
        size_t new_capacity = loop->break_capacity == 0 ? 8 : loop->break_capacity * 2;
        size_t* new_jumps = realloc(loop->break_jumps, new_capacity * sizeof(size_t));
        if (!new_jumps) {
            codegen_error(codegen, "Out of memory");
            return;
        }
        loop->break_jumps = new_jumps;
        loop->break_capacity = new_capacity;
    }
    
    loop->break_jumps[loop->break_count++] = jump_offset;
}

void codegen_emit_continue(codegen_t* codegen, ast_continue* node) {
    loop_context_t* loop = codegen_current_loop(codegen);
    if (!loop) {
        codegen_error(codegen, "Continue statement outside of loop");
        return;
    }
    
    // Calculate jump distance back to loop start
    size_t current_pos = codegen->chunk->count;
    size_t backward_distance = current_pos - loop->loop_start + 3; // +3 for the JUMP instruction itself
    
    if (backward_distance > UINT16_MAX) {
        codegen_error(codegen, "Loop body too large for continue");
        return;
    }
    
    // Emit backward jump to loop start
    codegen_emit_op_operand(codegen, OP_JUMP, (uint16_t)(-backward_distance));
}

// Get the current (innermost) loop context
loop_context_t* codegen_current_loop(codegen_t* codegen) {
    if (codegen->loop_depth == 0) return NULL;
    return &codegen->loop_contexts[codegen->loop_depth - 1];
}

// Push a new loop context onto the stack
void codegen_push_loop(codegen_t* codegen, size_t loop_start) {
    // Grow stack if needed
    if (codegen->loop_depth >= codegen->loop_capacity) {
        size_t new_capacity = codegen->loop_capacity == 0 ? 4 : codegen->loop_capacity * 2;
        loop_context_t* new_contexts = realloc(codegen->loop_contexts, 
                                              new_capacity * sizeof(loop_context_t));
        if (!new_contexts) {
            codegen_error(codegen, "Out of memory for loop context");
            return;
        }
        codegen->loop_contexts = new_contexts;
        codegen->loop_capacity = new_capacity;
    }
    
    // Initialize new loop context
    loop_context_t* loop = &codegen->loop_contexts[codegen->loop_depth];
    loop->loop_start = loop_start;
    loop->break_jumps = NULL;
    loop->break_count = 0;
    loop->break_capacity = 0;
    
    codegen->loop_depth++;
}

// Pop the current loop context from the stack and patch break statements
void codegen_pop_loop(codegen_t* codegen) {
    if (codegen->loop_depth == 0) {
        codegen_error(codegen, "Internal error: popping loop context when no loops active");
        return;
    }
    
    loop_context_t* loop = &codegen->loop_contexts[codegen->loop_depth - 1];
    
    // Patch all break statements to jump to current location
    for (size_t i = 0; i < loop->break_count; i++) {
        codegen_patch_jump(codegen, loop->break_jumps[i]);
    }
    
    // Clean up this loop context
    free(loop->break_jumps);
    
    codegen->loop_depth--;
}

void codegen_emit_return(codegen_t* codegen, ast_return* node) {
    if (node->value) {
        codegen_emit_expression(codegen, node->value);
    } else {
        codegen_emit_op(codegen, OP_PUSH_NULL);
    }
    
    codegen_emit_op(codegen, OP_RETURN);
}

// Utility functions
void codegen_emit_op(codegen_t* codegen, opcode op) {
    chunk_write_opcode(codegen->chunk, op);
}

void codegen_emit_op_operand(codegen_t* codegen, opcode op, uint16_t operand) {
    chunk_write_opcode(codegen->chunk, op);
    chunk_write_operand(codegen->chunk, operand);
}

// Helper function to get a specific line from source code
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
    
    // If we didn't find the line, return NULL
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

// Enhanced versions that add debug info from AST nodes
// Helper function to emit debug location instruction
void codegen_emit_debug_location(codegen_t* codegen, ast_node* node) {
    if (!codegen->debug_mode || !node || !codegen->chunk->debug || !codegen->chunk->debug->source_code) {
        return;
    }
    
    // Get the source line text
    size_t line_length;
    const char* line_start = get_source_line(codegen->chunk->debug->source_code, node->line, &line_length);
    if (!line_start) return;
    
    // Create a null-terminated copy of the line for storage as a string constant
    char* line_copy = malloc(line_length + 1);
    if (!line_copy) return;
    strncpy(line_copy, line_start, line_length);
    line_copy[line_length] = '\0';
    
    // Store the source line text as a string constant
    size_t constant_index = chunk_add_constant(codegen->chunk, make_string(line_copy));
    
    // For now, we'll store line and column in the operand (16 bits total)
    // Upper 8 bits = line number (limited to 255), lower 8 bits = column (limited to 255) 
    uint16_t location_info = ((node->line & 0xFF) << 8) | (node->column & 0xFF);
    
    // Emit the debug location instruction with the constant index
    codegen_emit_op_operand(codegen, OP_SET_DEBUG_LOCATION, (uint16_t)constant_index);
    
    // Follow with a special instruction that carries the line/column info
    chunk_write_byte(codegen->chunk, (location_info >> 8) & 0xFF); // line
    chunk_write_byte(codegen->chunk, location_info & 0xFF); // column
    
    free(line_copy);
}

void codegen_emit_op_with_debug(codegen_t* codegen, opcode op, ast_node* node) {
    if (codegen->debug_mode && node) {
        // Emit debug location instruction before the actual operation
        codegen_emit_debug_location(codegen, node);
        chunk_add_debug_info(codegen->chunk, node->line, node->column);
    }
    chunk_write_opcode(codegen->chunk, op);
}

void codegen_emit_op_operand_with_debug(codegen_t* codegen, opcode op, uint16_t operand, ast_node* node) {
    if (codegen->debug_mode && node) {
        chunk_add_debug_info(codegen->chunk, node->line, node->column);
    }
    chunk_write_opcode(codegen->chunk, op);
    chunk_write_operand(codegen->chunk, operand);
}

size_t codegen_emit_jump(codegen_t* codegen, opcode op) {
    codegen_emit_op_operand(codegen, op, 0xFFFF); // Placeholder
    return codegen->chunk->count - 2; // Return offset of operand
}

void codegen_patch_jump(codegen_t* codegen, size_t offset) {
    // Calculate jump distance
    size_t jump = codegen->chunk->count - offset - 2;
    
    if (jump > UINT16_MAX) {
        codegen_error(codegen, "Too much code to jump over");
        return;
    }
    
    // Patch the jump operand
    codegen->chunk->code[offset] = (uint8_t)(jump & 0xFF);
    codegen->chunk->code[offset + 1] = (uint8_t)((jump >> 8) & 0xFF);
}

void codegen_emit_loop(codegen_t* codegen, size_t loop_start) {
    size_t offset = codegen->chunk->count - loop_start + 3; // +3 for the LOOP instruction itself
    
    if (offset > UINT16_MAX) {
        codegen_error(codegen, "Loop body too large");
        return;
    }
    
    codegen_emit_op_operand(codegen, OP_LOOP, (uint16_t)offset);
}

// Error handling
void codegen_error(codegen_t* codegen, const char* message) {
    fprintf(stderr, "Codegen error: %s\n", message);
    codegen->had_error = 1;
}

// Debug functions
void chunk_disassemble(bytecode_chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    
    for (size_t offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

size_t disassemble_instruction(bytecode_chunk* chunk, size_t offset) {
    printf("%04zu ", offset);
    
    uint8_t instruction = chunk->code[offset];
    
    switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            printf("%-16s %4d ", opcode_name(instruction), constant);
            if (constant >= chunk->constant_count) {
                printf("'[INVALID INDEX - max: %zu]'\n", chunk->constant_count - 1);
            } else {
                printf("'");
                print_value(NULL, chunk->constants[constant]);
                printf("'\n");
            }
            return offset + 3;
        }
        
        case OP_BUILD_ARRAY:
        case OP_BUILD_OBJECT:
        case OP_CALL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_DEFINE_GLOBAL:
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP_IF_TRUE:
        case OP_LOOP: {
            uint16_t operand = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            printf("%-16s %4d\n", opcode_name(instruction), operand);
            return offset + 3;
        }
        
        case OP_SET_DEBUG_LOCATION: {
            uint16_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            uint8_t line = chunk->code[offset + 3];
            uint8_t column = chunk->code[offset + 4];
            printf("%-16s %4d (line %d, col %d)\n", opcode_name(instruction), constant, line, column);
            return offset + 5;
        }
        
        case OP_CLEAR_DEBUG_LOCATION:
            printf("%-16s\n", opcode_name(instruction));
            return offset + 1;
        
        default:
            printf("%s\n", opcode_name(instruction));
            return offset + 1;
    }
}