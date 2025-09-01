#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Control flow emission functions
void codegen_emit_while(codegen_t* codegen, ast_while* node) {
    size_t loop_start = codegen->chunk->count;
    
    // Begin loop context for break and continue statements
    codegen_push_loop(codegen, LOOP_WHILE, loop_start);
    
    // Begin scope for the while loop body
    codegen_begin_scope(codegen);
    
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false (exit loop) - OP_JUMP_IF_FALSE pops condition automatically
    size_t exit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    
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
    
    // Patch exit jump (no need to pop condition - OP_JUMP_IF_FALSE already did)
    codegen_patch_jump(codegen, exit_jump);
    
    // End scope for the while loop
    codegen_end_scope(codegen);
    
    // End loop context and patch break statements
    codegen_pop_loop(codegen);
}

// Emit for loop with dual scoping: outer scope for initializer, inner scope for body
void codegen_emit_for(codegen_t* codegen, ast_for* node) {
    // BEGIN OUTER SCOPE for initializer variables
    codegen_begin_scope(codegen);
    
    // Emit initializer (if present) - variables live in outer scope
    if (node->initializer) {
        codegen_emit_statement(codegen, node->initializer);
    }
    
    // Mark loop start for continue statements and backward jump
    size_t loop_start = codegen->chunk->count;
    
    // Begin loop context for break and continue statements
    codegen_push_loop(codegen, LOOP_FOR, loop_start);
    
    // BEGIN INNER SCOPE for loop body
    codegen_begin_scope(codegen);
    
    // Emit condition check (defaults to true if absent)
    size_t exit_jump = 0;
    if (node->condition) {
        codegen_emit_expression(codegen, node->condition);
        // Jump if false (exit loop) - OP_JUMP_IF_FALSE pops condition automatically
        exit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    }
    
    // Emit loop body within inner scope
    codegen_emit_statement(codegen, node->body);
    
    // Patch continue jumps to jump to increment section
    loop_context_t* current_loop = codegen_current_loop(codegen);
    if (current_loop && current_loop->continue_jumps) {
        for (size_t i = 0; i < current_loop->continue_jump_count; i++) {
            codegen_patch_jump(codegen, current_loop->continue_jumps[i]);
        }
    }
    
    // Emit increment expression (if present) - accesses outer scope variables
    if (node->increment) {
        codegen_emit_expression(codegen, node->increment);
        codegen_emit_op(codegen, OP_POP); // Discard increment result
    }
    
    // Jump back to loop start (condition check)
    size_t current_pos = codegen->chunk->count;
    size_t backward_distance = current_pos - loop_start + 3; // +3 for the JUMP instruction itself
    if (backward_distance > UINT16_MAX) {
        codegen_error(codegen, "For loop body too large");
        return;
    }
    // Emit backward jump (negative offset)
    codegen_emit_op_operand(codegen, OP_JUMP, (uint16_t)(-backward_distance));
    
    // Patch exit jump (if condition was present) - no need to pop condition
    if (node->condition) {
        codegen_patch_jump(codegen, exit_jump);
    }
    
    // END INNER SCOPE (cleans up loop body variables)
    codegen_end_scope(codegen);
    
    // End loop context and patch break statements
    codegen_pop_loop(codegen);
    
    // END OUTER SCOPE (cleans up initializer variables) 
    codegen_end_scope(codegen);
}

void codegen_emit_do_while(codegen_t* codegen, ast_do_while* node) {
    size_t loop_start = codegen->chunk->count;
    
    // Begin loop context for break and continue statements
    // For do-while, continue should jump to the condition check (after body)
    // We'll adjust this after we know where the condition starts
    codegen_push_loop(codegen, LOOP_DO_WHILE, loop_start);
    
    // Begin scope for the do-while loop body
    codegen_begin_scope(codegen);
    
    // Generate body first (do-while executes body at least once)
    codegen_emit_statement(codegen, node->body);
    
    // Update loop context for continue statements (they should jump to condition check)
    loop_context_t* current_loop = codegen_current_loop(codegen);
    if (current_loop) {
        // For do-while, continues should jump to the condition check, not the body start
        current_loop->continue_target = codegen->chunk->count;
    }
    
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump to exit if condition is false - OP_JUMP_IF_FALSE pops condition automatically
    size_t exit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    
    // Jump back to start of loop body (unconditional backward jump)
    codegen_emit_loop(codegen, loop_start);
    
    // Patch the exit jump
    codegen_patch_jump(codegen, exit_jump);
    
    // End scope for the do-while loop
    codegen_end_scope(codegen);
    
    // End loop context and patch break statements
    codegen_pop_loop(codegen);
}

void codegen_emit_infinite_loop(codegen_t* codegen, ast_loop* node) {
    size_t loop_start = codegen->chunk->count;
    
    // Begin loop context for break and continue statements
    codegen_push_loop(codegen, LOOP_INFINITE, loop_start);
    
    // Begin scope for the infinite loop body
    codegen_begin_scope(codegen);
    
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
    
    // End scope for the infinite loop
    codegen_end_scope(codegen);
    
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
    
    if (loop->type == LOOP_FOR) {
        // For for-loops, emit forward jump to increment section
        // Store this jump to be patched when we reach the increment section
        size_t jump_offset = codegen_emit_jump(codegen, OP_JUMP);
        
        // Grow continue jumps array if needed
        if (loop->continue_jump_count >= loop->continue_jump_capacity) {
            size_t new_capacity = loop->continue_jump_capacity == 0 ? 2 : loop->continue_jump_capacity * 2;
            size_t* new_jumps = realloc(loop->continue_jumps, new_capacity * sizeof(size_t));
            if (!new_jumps) {
                codegen_error(codegen, "Out of memory for continue jump");
                return;
            }
            loop->continue_jumps = new_jumps;
            loop->continue_jump_capacity = new_capacity;
        }
        
        // Store the jump for patching to increment section
        loop->continue_jumps[loop->continue_jump_count++] = jump_offset;
    } else {
        // For while/do-while/infinite loops, jump back to continue_target
        size_t current_pos = codegen->chunk->count;
        size_t backward_distance = current_pos - loop->continue_target + 3;
        
        if (backward_distance > UINT16_MAX) {
            codegen_error(codegen, "Loop body too large for continue");
            return;
        }
        
        codegen_emit_op_operand(codegen, OP_JUMP, (uint16_t)(-backward_distance));
    }
}

// Get the current (innermost) loop context
loop_context_t* codegen_current_loop(codegen_t* codegen) {
    if (codegen->loop_depth == 0) return NULL;
    return &codegen->loop_contexts[codegen->loop_depth - 1];
}

// Push a new loop context onto the stack
void codegen_push_loop(codegen_t* codegen, loop_type_t type, size_t loop_start) {
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
    loop->type = type;
    loop->loop_start = loop_start;
    loop->continue_target = loop_start;  // Default to loop_start, may be updated
    loop->break_jumps = NULL;
    loop->break_count = 0;
    loop->break_capacity = 0;
    loop->continue_jumps = NULL;
    loop->continue_jump_count = 0;
    loop->continue_jump_capacity = 0;
    
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
    free(loop->continue_jumps);
    
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