#include "codegen.h"
#include "runtime_error.h"
#include "library_assert.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Operator emission functions
void codegen_emit_binary_op(codegen_t* codegen, ast_binary_op* node) {
    // Special handling for short-circuit boolean operators
    if (node->op == BIN_LOGICAL_AND) {
        // Generate left operand
        codegen_emit_expression(codegen, node->left);
        // Duplicate left operand (one for condition, one as potential result)
        codegen_emit_op(codegen, OP_DUP);
        // Jump if false, keeping left operand as result
        size_t short_circuit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
        // Pop the duplicated left operand (condition was true)
        codegen_emit_op(codegen, OP_POP);
        // Generate right operand (becomes the result)
        codegen_emit_expression(codegen, node->right);
        // Patch the short-circuit jump
        codegen_patch_jump(codegen, short_circuit_jump);
        return;
    }
    
    if (node->op == BIN_LOGICAL_OR) {
        // Generate left operand
        codegen_emit_expression(codegen, node->left);
        // Duplicate left operand (one for condition, one as potential result)
        codegen_emit_op(codegen, OP_DUP);
        // Jump if true, keeping left operand as result
        size_t short_circuit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_TRUE);
        // Pop the duplicated left operand (condition was false)
        codegen_emit_op(codegen, OP_POP);
        // Generate right operand (becomes the result)
        codegen_emit_expression(codegen, node->right);
        // Patch the short-circuit jump
        codegen_patch_jump(codegen, short_circuit_jump);
        return;
    }
    
    // For all other operators, generate both operands first
    codegen_emit_expression(codegen, node->left);
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
        case BIN_BITWISE_AND:   codegen_emit_op(codegen, OP_BITWISE_AND); break;
        case BIN_BITWISE_OR:    codegen_emit_op(codegen, OP_BITWISE_OR); break;
        case BIN_BITWISE_XOR:   codegen_emit_op(codegen, OP_BITWISE_XOR); break;
        case BIN_LEFT_SHIFT:    codegen_emit_op(codegen, OP_LEFT_SHIFT); break;
        case BIN_RIGHT_SHIFT:   codegen_emit_op(codegen, OP_RIGHT_SHIFT); break;
        case BIN_LOGICAL_RIGHT_SHIFT: codegen_emit_op(codegen, OP_LOGICAL_RIGHT_SHIFT); break;
        case BIN_FLOOR_DIV:     codegen_emit_op(codegen, OP_FLOOR_DIV); break;
        case BIN_NULL_COALESCE: codegen_emit_op(codegen, OP_NULL_COALESCE); break;
        case BIN_IN:            codegen_emit_op(codegen, OP_IN); break;
        case BIN_INSTANCEOF:    codegen_emit_op(codegen, OP_INSTANCEOF); break;
    }
}

void codegen_emit_ternary(codegen_t* codegen, ast_ternary* node) {
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false to false branch (OP_JUMP_IF_FALSE pops the condition automatically)
    size_t false_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    
    // Generate true expression
    codegen_emit_expression(codegen, node->true_expr);
    
    // Jump over false branch
    size_t end_jump = codegen_emit_jump(codegen, OP_JUMP);
    
    // Patch false jump (no need to pop condition - OP_JUMP_IF_FALSE already did)
    codegen_patch_jump(codegen, false_jump);
    
    // Generate false expression
    codegen_emit_expression(codegen, node->false_expr);
    
    // Patch end jump
    codegen_patch_jump(codegen, end_jump);
}

void codegen_emit_range(codegen_t* codegen, ast_range* node) {
    // Emit debug location before building range
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    // Generate start value
    codegen_emit_expression(codegen, node->start);
    
    // Generate end value
    codegen_emit_expression(codegen, node->end);
    
    // Generate step value (or default if not specified)
    if (node->step) {
        codegen_emit_expression(codegen, node->step);
    } else {
        // Default step: use INT32(1) for auto-detection
        codegen_emit_debug_location(codegen, (ast_node*)node);
        size_t constant = chunk_add_constant(codegen->chunk, make_int32(1));
        codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
    }
    
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
        case AST_CALL:
            // Function-call style indexing is an l-value (e.g., arr(0)++)
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
            // Suppress error messages in test context, but still set error flag
            if (g_current_vm && g_current_vm->context != CTX_TEST) {
                printf("Compile error: %s operator can only be applied to l-values (variables, array elements, object properties)\n",
                       (node->op == UN_PRE_INCREMENT || node->op == UN_POST_INCREMENT) ? "Increment" : "Decrement");
            }
            codegen->had_error = 1;
            return;
        }
        
        // Handle increment/decrement operators specially - they need to modify the variable
        if (node->operand->type == AST_IDENTIFIER) {
            ast_identifier* var = (ast_identifier*)node->operand;
            
            // Check if it's local, upvalue, or global
            int is_local;
            int upvalue_index;
            int slot = codegen_resolve_variable(codegen, var->name, &is_local, &upvalue_index);
            
            if (is_local) {
                // Local variable increment/decrement
                switch (node->op) {
                    case UN_PRE_INCREMENT:
                        // Load, increment, store, leave new value on stack
                        codegen_emit_op(codegen, OP_GET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        codegen_emit_op(codegen, OP_INCREMENT);
                        codegen_emit_op(codegen, OP_DUP);  // Duplicate for return value
                        codegen_emit_op(codegen, OP_SET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        break;
                    case UN_PRE_DECREMENT:
                        // Load, decrement, store, leave new value on stack
                        codegen_emit_op(codegen, OP_GET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        codegen_emit_op(codegen, OP_DECREMENT);
                        codegen_emit_op(codegen, OP_DUP);  // Duplicate for return value
                        codegen_emit_op(codegen, OP_SET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        break;
                    case UN_POST_INCREMENT:
                        // Load, duplicate (for return), increment, store
                        codegen_emit_op(codegen, OP_GET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        codegen_emit_op(codegen, OP_DUP);  // Original value for return
                        codegen_emit_op(codegen, OP_INCREMENT);
                        codegen_emit_op(codegen, OP_SET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        break;
                    case UN_POST_DECREMENT:
                        // Load, duplicate (for return), decrement, store
                        codegen_emit_op(codegen, OP_GET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        codegen_emit_op(codegen, OP_DUP);  // Original value for return
                        codegen_emit_op(codegen, OP_DECREMENT);
                        codegen_emit_op(codegen, OP_SET_LOCAL);
                        chunk_write_byte(codegen->chunk, (uint8_t)slot);
                        break;
                    default: break;
                }
            } else if (upvalue_index != -1) {
                // Upvalue increment/decrement
                switch (node->op) {
                    case UN_PRE_INCREMENT:
                        // Load, increment, store, leave new value on stack
                        codegen_emit_op(codegen, OP_GET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        codegen_emit_op(codegen, OP_INCREMENT);
                        codegen_emit_op(codegen, OP_DUP);  // Duplicate for return value
                        codegen_emit_op(codegen, OP_SET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        break;
                    case UN_PRE_DECREMENT:
                        // Load, decrement, store, leave new value on stack
                        codegen_emit_op(codegen, OP_GET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        codegen_emit_op(codegen, OP_DECREMENT);
                        codegen_emit_op(codegen, OP_DUP);  // Duplicate for return value
                        codegen_emit_op(codegen, OP_SET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        break;
                    case UN_POST_INCREMENT:
                        // Load, duplicate (for return), increment, store
                        codegen_emit_op(codegen, OP_GET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        codegen_emit_op(codegen, OP_DUP);  // Original value for return
                        codegen_emit_op(codegen, OP_INCREMENT);
                        codegen_emit_op(codegen, OP_SET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        break;
                    case UN_POST_DECREMENT:
                        // Load, duplicate (for return), decrement, store
                        codegen_emit_op(codegen, OP_GET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        codegen_emit_op(codegen, OP_DUP);  // Original value for return
                        codegen_emit_op(codegen, OP_DECREMENT);
                        codegen_emit_op(codegen, OP_SET_UPVALUE);
                        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
                        break;
                    default: break;
                }
            } else {
                // Global variable increment/decrement
                size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
                
                switch (node->op) {
                    case UN_PRE_INCREMENT:
                        // Load, increment, store, leave new value on stack
                        codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
                        codegen_emit_op(codegen, OP_INCREMENT);
                        codegen_emit_op(codegen, OP_DUP);  // Duplicate for return value
                        codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
                        break;
                    case UN_PRE_DECREMENT:
                        // Load, decrement, store, leave new value on stack
                        codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
                        codegen_emit_op(codegen, OP_DECREMENT);
                        codegen_emit_op(codegen, OP_DUP);  // Duplicate for return value
                        codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
                        break;
                    case UN_POST_INCREMENT:
                        // Load, duplicate (for return), increment, store
                        codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
                        codegen_emit_op(codegen, OP_DUP);  // Original value for return
                        codegen_emit_op(codegen, OP_INCREMENT);
                        codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
                        break;
                    case UN_POST_DECREMENT:
                        // Load, duplicate (for return), decrement, store
                        codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
                        codegen_emit_op(codegen, OP_DUP);  // Original value for return
                        codegen_emit_op(codegen, OP_DECREMENT);
                        codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
                        break;
                    default: break;
                }
            }
        } else if (node->operand->type == AST_MEMBER) {
            // Object property increment/decrement: ++obj.prop, obj.prop++, --obj.prop, obj.prop--
            ast_member* member = (ast_member*)node->operand;
            size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member->property));
            
            switch (node->op) {
                case UN_PRE_INCREMENT:
                    // Pre-increment: get value, increment, duplicate for return, set
                    codegen_emit_expression(codegen, member->object);  // [object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [object, property]
                    codegen_emit_op(codegen, OP_GET_PROPERTY);  // [value]
                    codegen_emit_op(codegen, OP_INCREMENT);     // [new_value]
                    codegen_emit_op(codegen, OP_DUP);           // [new_value, new_value]
                    
                    // Prepare for SET_PROPERTY: need [object, property, new_value]
                    codegen_emit_expression(codegen, member->object);  // [new_value, new_value, object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [new_value, new_value, object, property]
                    codegen_emit_op(codegen, OP_ROT);           // [new_value, object, property, new_value]
                    codegen_emit_op(codegen, OP_SET_PROPERTY);  // [new_value]
                    break;
                    
                case UN_PRE_DECREMENT:
                    // Pre-decrement: get value, decrement, duplicate for return, set
                    codegen_emit_expression(codegen, member->object);  // [object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [object, property]
                    codegen_emit_op(codegen, OP_GET_PROPERTY);  // [value]
                    codegen_emit_op(codegen, OP_DECREMENT);     // [new_value]
                    codegen_emit_op(codegen, OP_DUP);           // [new_value, new_value]
                    
                    // Prepare for SET_PROPERTY: need [object, property, new_value]
                    codegen_emit_expression(codegen, member->object);  // [new_value, new_value, object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [new_value, new_value, object, property]
                    codegen_emit_op(codegen, OP_ROT);           // [new_value, object, property, new_value]
                    codegen_emit_op(codegen, OP_SET_PROPERTY);  // [new_value]
                    break;
                    
                case UN_POST_INCREMENT:
                    // Post-increment: get value, duplicate for return, increment, set
                    codegen_emit_expression(codegen, member->object);  // [object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [object, property]
                    codegen_emit_op(codegen, OP_GET_PROPERTY);  // [old_value]
                    codegen_emit_op(codegen, OP_DUP);           // [old_value, old_value]
                    codegen_emit_op(codegen, OP_INCREMENT);     // [old_value, new_value]
                    
                    // Prepare for SET_PROPERTY: need [object, property, new_value], then restore [old_value]
                    codegen_emit_expression(codegen, member->object);  // [old_value, new_value, object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [old_value, new_value, object, property]
                    codegen_emit_op(codegen, OP_ROT);           // [old_value, object, property, new_value]
                    codegen_emit_op(codegen, OP_SET_PROPERTY);  // [old_value, new_value] - SET_PROPERTY returns assigned value
                    codegen_emit_op(codegen, OP_POP);           // [old_value] - discard returned value, keep old_value
                    break;
                    
                case UN_POST_DECREMENT:
                    // Post-decrement: get value, duplicate for return, decrement, set
                    codegen_emit_expression(codegen, member->object);  // [object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [object, property]
                    codegen_emit_op(codegen, OP_GET_PROPERTY);  // [old_value]
                    codegen_emit_op(codegen, OP_DUP);           // [old_value, old_value]
                    codegen_emit_op(codegen, OP_DECREMENT);     // [old_value, new_value]
                    
                    // Prepare for SET_PROPERTY: need [object, property, new_value], then restore [old_value]
                    codegen_emit_expression(codegen, member->object);  // [old_value, new_value, object]
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);  // [old_value, new_value, object, property]
                    codegen_emit_op(codegen, OP_ROT);           // [old_value, object, property, new_value]
                    codegen_emit_op(codegen, OP_SET_PROPERTY);  // [old_value, new_value] - SET_PROPERTY returns assigned value  
                    codegen_emit_op(codegen, OP_POP);           // [old_value] - discard returned value, keep old_value
                    break;
                    
                default: break;
            }
            
        } else if (node->operand->type == AST_CALL) {
            // Array element increment/decrement: ++arr(i), arr(i)++, --arr(i), arr(i)--  
            ast_call* call = (ast_call*)node->operand;
            
            if (call->arg_count != 1) {
                if (g_current_vm && g_current_vm->context != CTX_TEST) {
                    printf("Compile error: Array increment/decrement requires exactly one index\n");
                }
                codegen->had_error = 1;
                return;
            }
            
            switch (node->op) {
                case UN_PRE_INCREMENT:
                    // Pre-increment: get value using OP_CALL, increment, duplicate for return, set using OP_SET_INDEX
                    // First, get the current value by calling the array
                    codegen_emit_expression(codegen, call->function);     // [array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [array, index]
                    codegen_emit_op_operand(codegen, OP_CALL, 1);         // [value]
                    codegen_emit_op(codegen, OP_INCREMENT);               // [new_value]
                    codegen_emit_op(codegen, OP_DUP);                     // [new_value, new_value]
                    
                    // Setup for OP_SET_INDEX: need [array, index, new_value] 
                    codegen_emit_expression(codegen, call->function);     // [new_value, new_value, array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [new_value, new_value, array, index]
                    codegen_emit_op(codegen, OP_ROT);                     // [new_value, array, index, new_value]
                    codegen_emit_op(codegen, OP_SET_INDEX);               // [new_value]
                    break;
                    
                case UN_PRE_DECREMENT:
                    // Pre-decrement: get value using OP_CALL, decrement, duplicate for return, set using OP_SET_INDEX
                    codegen_emit_expression(codegen, call->function);     // [array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [array, index]
                    codegen_emit_op_operand(codegen, OP_CALL, 1);         // [value]
                    codegen_emit_op(codegen, OP_DECREMENT);               // [new_value]
                    codegen_emit_op(codegen, OP_DUP);                     // [new_value, new_value]
                    
                    // Setup for OP_SET_INDEX: need [array, index, new_value] 
                    codegen_emit_expression(codegen, call->function);     // [new_value, new_value, array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [new_value, new_value, array, index]
                    codegen_emit_op(codegen, OP_ROT);                     // [new_value, array, index, new_value]
                    codegen_emit_op(codegen, OP_SET_INDEX);               // [new_value]
                    break;
                    
                case UN_POST_INCREMENT:
                    // Post-increment: get value using OP_CALL, duplicate for return, increment, set using OP_SET_INDEX
                    codegen_emit_expression(codegen, call->function);     // [array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [array, index]
                    codegen_emit_op_operand(codegen, OP_CALL, 1);         // [old_value]
                    codegen_emit_op(codegen, OP_DUP);                     // [old_value, old_value]
                    codegen_emit_op(codegen, OP_INCREMENT);               // [old_value, new_value]
                    
                    // Setup for OP_SET_INDEX: need [array, index, new_value], then restore [old_value] 
                    codegen_emit_expression(codegen, call->function);     // [old_value, new_value, array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [old_value, new_value, array, index]
                    codegen_emit_op(codegen, OP_ROT);                     // [old_value, array, index, new_value]
                    codegen_emit_op(codegen, OP_SET_INDEX);               // [old_value, new_value] - OP_SET_INDEX returns assigned value
                    codegen_emit_op(codegen, OP_POP);                     // [old_value] - discard the returned new_value, keep old_value
                    break;
                    
                case UN_POST_DECREMENT:
                    // Post-decrement: get value using OP_CALL, duplicate for return, decrement, set using OP_SET_INDEX
                    codegen_emit_expression(codegen, call->function);     // [array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [array, index]
                    codegen_emit_op_operand(codegen, OP_CALL, 1);         // [old_value]
                    codegen_emit_op(codegen, OP_DUP);                     // [old_value, old_value]
                    codegen_emit_op(codegen, OP_DECREMENT);               // [old_value, new_value]
                    
                    // Setup for OP_SET_INDEX: need [array, index, new_value], then restore [old_value]
                    codegen_emit_expression(codegen, call->function);     // [old_value, new_value, array]
                    codegen_emit_expression(codegen, call->arguments[0]); // [old_value, new_value, array, index]
                    codegen_emit_op(codegen, OP_ROT);                     // [old_value, array, index, new_value]
                    codegen_emit_op(codegen, OP_SET_INDEX);               // [old_value, new_value] - OP_SET_INDEX returns assigned value
                    codegen_emit_op(codegen, OP_POP);                     // [old_value] - discard the returned new_value, keep old_value
                    break;
                    
                default: break;
            }
            
        } else {
            // Unknown l-value type
            if (g_current_vm && g_current_vm->context != CTX_TEST) {
                printf("Compile error: Increment/decrement can only be applied to l-values (variables, array elements, object properties)\n");
            }
            codegen->had_error = 1;
            return;
        }
        return;
    }
    
    // Generate operand for other unary operators
    codegen_emit_expression(codegen, node->operand);
    
    // Generate operator
    switch (node->op) {
        case UN_NEGATE: codegen_emit_op(codegen, OP_NEGATE); break;
        case UN_NOT:    codegen_emit_op(codegen, OP_NOT); break;
        case UN_BITWISE_NOT: codegen_emit_op(codegen, OP_BITWISE_NOT); break;
        default: break; // increment/decrement handled above
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