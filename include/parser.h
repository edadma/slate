#ifndef BIT_PARSER_H
#define BIT_PARSER_H

#include "lexer.h"
#include "ast.h"

// Parser state
typedef struct {
    lexer_t* lexer;
    token_t current;
    token_t previous;
    int had_error;
    int panic_mode;
} parser_t;

// Parser functions
void parser_init(parser_t* parser, lexer_t* lexer);
ast_program* parse_program(parser_t* parser);

// Error handling
void parser_error_at(parser_t* parser, token_t* token, const char* message);
void parser_error(parser_t* parser, const char* message);
void parser_error_at_current(parser_t* parser, const char* message);

// Token consumption
void parser_advance(parser_t* parser);
int parser_check(parser_t* parser, token_type_t type);
int parser_match(parser_t* parser, token_type_t type);
void parser_consume(parser_t* parser, token_type_t type, const char* message);
void parser_synchronize(parser_t* parser);

// Parsing functions (recursive descent)

// Statements
ast_node* parse_statement(parser_t* parser);
ast_node* parse_declaration(parser_t* parser);
ast_node* parse_var_declaration(parser_t* parser);
ast_node* parse_expression_statement(parser_t* parser);
ast_node* parse_if_expression(parser_t* parser);
ast_node* parse_indented_block(parser_t* parser);
int validate_block_expression(ast_node* expr);
ast_node* parse_while_statement(parser_t* parser);
ast_node* parse_do_while_statement(parser_t* parser);
ast_node* parse_loop_statement(parser_t* parser);
ast_node* parse_break_statement(parser_t* parser);
ast_node* parse_continue_statement(parser_t* parser);
ast_node* parse_return_statement(parser_t* parser);

// Expressions (by precedence)
ast_node* parse_expression(parser_t* parser);
ast_node* parse_assignment(parser_t* parser);
ast_node* parse_or(parser_t* parser);
ast_node* parse_and(parser_t* parser);
ast_node* parse_equality(parser_t* parser);
ast_node* parse_comparison(parser_t* parser);
ast_node* parse_range(parser_t* parser);
ast_node* parse_term(parser_t* parser);
ast_node* parse_factor(parser_t* parser);
ast_node* parse_unary(parser_t* parser);
ast_node* parse_call(parser_t* parser);
ast_node* parse_primary(parser_t* parser);

// Literals and complex expressions
ast_node* parse_number(parser_t* parser);
ast_node* parse_string(parser_t* parser);
ast_node* parse_boolean(parser_t* parser);
ast_node* parse_null(parser_t* parser);
ast_node* parse_identifier(parser_t* parser);
ast_node* parse_array(parser_t* parser);
ast_node* parse_object(parser_t* parser);
ast_node* parse_function(parser_t* parser);

// Utility functions
ast_node* finish_call(parser_t* parser, ast_node* callee);
binary_operator token_to_binary_op(token_type_t type);
unary_operator token_to_unary_op(token_type_t type);

#endif // BIT_PARSER_H