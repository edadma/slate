#ifndef SLATE_PARSER_INTERNAL_H
#define SLATE_PARSER_INTERNAL_H

#include "parser.h"
#include "lexer.h"
#include "ast.h"

// Parser utilities (shared across all modules)
char* token_to_string(token_t* token);
double token_to_number(token_t* token);
float token_to_float32(token_t* token);
binary_operator token_to_binary_op(token_type_t type);
unary_operator token_to_unary_op(token_type_t type);

// Parser core functions (shared across all modules)
void parser_error_at(parser_t* parser, token_t* token, const char* message);
void parser_error(parser_t* parser, const char* message);
void parser_error_at_current(parser_t* parser, const char* message);
void parser_advance(parser_t* parser);
int parser_check(parser_t* parser, token_type_t type);
int parser_match(parser_t* parser, token_type_t type);
void parser_consume(parser_t* parser, token_type_t type, const char* message);
void parser_pushback(parser_t* parser);
void parser_synchronize(parser_t* parser);

// Declaration parsing (declarations.c)
ast_node* parse_declaration(parser_t* parser);
ast_node* parse_var_declaration(parser_t* parser);
ast_node* parse_def_declaration(parser_t* parser);

// Statement parsing (statements.c)
ast_node* parse_statement(parser_t* parser);
ast_node* parse_indented_block(parser_t* parser);
ast_node* parse_if_expression(parser_t* parser);
int validate_block_expression(ast_node* expr, parser_mode_t mode);
ast_node* parse_while_statement(parser_t* parser);
ast_node* parse_for_statement(parser_t* parser);
ast_node* parse_do_while_statement(parser_t* parser);
ast_node* parse_loop_statement(parser_t* parser);
ast_node* parse_break_statement(parser_t* parser);
ast_node* parse_continue_statement(parser_t* parser);
ast_node* parse_return_statement(parser_t* parser);
ast_node* parse_expression_statement(parser_t* parser);

// Expression parsing (expressions.c)
ast_node* parse_lambda_or_assignment(parser_t* parser);
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
ast_node* parse_postfix(parser_t* parser);
ast_node* parse_call(parser_t* parser);
ast_node* finish_call(parser_t* parser, ast_node* callee);
ast_node* parse_primary(parser_t* parser);
ast_node* parse_arrow_function(parser_t* parser, char** parameters, size_t param_count);
ast_node* parse_parenthesized_or_arrow(parser_t* parser);
ast_node* parse_array(parser_t* parser);
ast_node* parse_object(parser_t* parser);

#endif // SLATE_PARSER_INTERNAL_H