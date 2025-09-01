#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "../include/codegen.h"

// Internal forward declarations between codegen modules
// These functions are used across multiple codegen files but not exposed in the public API

// Forward declarations for cross-module dependencies
void codegen_emit_template_literal(codegen_t* codegen, ast_template_literal* node);
bool is_lvalue(ast_node* node);
loop_context_t* codegen_current_loop(codegen_t* codegen);
void codegen_push_loop(codegen_t* codegen, loop_type_t type, size_t loop_start);
void codegen_pop_loop(codegen_t* codegen);

#endif // CODEGEN_INTERNAL_H