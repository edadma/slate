#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>

// Include dynamic_int.h for BigInt support
#include "dynamic_int.h"
#include "value.h"

// Forward declarations
typedef struct bit_object bit_object;

// AST Node types
typedef enum {
    // Literals
    AST_INTEGER, // 32-bit integer literal
    AST_BIGINT, // Arbitrary precision integer literal
    AST_NUMBER, // Floating point literal (precision determined at parse time)
    AST_STRING,
    AST_TEMPLATE_LITERAL, // Template literal with interpolation
    AST_BOOLEAN,
    AST_NULL,
    AST_UNDEFINED,
    AST_IDENTIFIER,
    AST_ARRAY,

    // Binary operations
    AST_BINARY_OP,

    // Ternary conditional
    AST_TERNARY,

    // Range expressions
    AST_RANGE,

    // Unary operations
    AST_UNARY_OP,

    // Function expressions
    AST_FUNCTION,
    AST_CALL,

    // Object/property access
    AST_MEMBER,
    AST_OBJECT_LITERAL,

    // Statements
    AST_VAR_DECLARATION,
    AST_ASSIGNMENT,
    AST_COMPOUND_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_DO_WHILE,
    AST_FOR,
    AST_LOOP,
    AST_BREAK,
    AST_CONTINUE,
    AST_RETURN,
    AST_EXPRESSION_STMT,
    AST_BLOCK,

    // Program
    AST_PROGRAM
} ast_node_type;

// Binary operators
typedef enum {
    BIN_ADD, // +
    BIN_SUBTRACT, // -
    BIN_MULTIPLY, // *
    BIN_DIVIDE, // /
    BIN_MOD, // mod
    BIN_POWER, // **
    BIN_EQUAL, // ==
    BIN_NOT_EQUAL, // !=
    BIN_LESS, // <
    BIN_LESS_EQUAL, // <=
    BIN_GREATER, // >
    BIN_GREATER_EQUAL, // >=
    BIN_LOGICAL_AND, // &&
    BIN_LOGICAL_OR, // ||
    BIN_BITWISE_AND, // &
    BIN_BITWISE_OR, // |
    BIN_BITWISE_XOR, // ^
    BIN_LEFT_SHIFT, // <<
    BIN_RIGHT_SHIFT, // >> (arithmetic)
    BIN_LOGICAL_RIGHT_SHIFT, // >>> (logical)
    BIN_FLOOR_DIV, // //
    BIN_NULL_COALESCE, // ??
    BIN_IN, // in (property existence)
    BIN_INSTANCEOF // instanceof (type checking)
} binary_operator;

// Unary operators
typedef enum {
    UN_NEGATE, // -
    UN_NOT, // !
    UN_BITWISE_NOT, // ~
    UN_PRE_INCREMENT, // ++x
    UN_PRE_DECREMENT, // --x
    UN_POST_INCREMENT, // x++
    UN_POST_DECREMENT // x--
} unary_operator;

// Base AST node structure
typedef struct ast_node {
    ast_node_type type;
    int line; // Source line number for error reporting
    int column; // Source column number for error reporting
} ast_node;

// Literal nodes
typedef struct {
    ast_node base;
    int32_t value;
} ast_integer;

typedef struct {
    ast_node base;
    di_int value; // Arbitrary precision integer
} ast_bigint;

typedef struct {
    ast_node base;
    union {
        float float32;
        double float64; 
    } value;
    int is_float32; // 1 if float32, 0 if float64
} ast_number;

typedef struct {
    ast_node base;
    char* value; // Null-terminated string (we'll use dynamic_string.h later)
} ast_string;

// Template literal part - either text or expression
typedef enum {
    TEMPLATE_PART_TEXT, // Static text segment
    TEMPLATE_PART_EXPRESSION // Interpolated expression (${expr} or $var)
} template_part_type;

typedef struct {
    template_part_type type;
    union {
        char* text; // For TEMPLATE_PART_TEXT
        ast_node* expression; // For TEMPLATE_PART_EXPRESSION
    } as;
} template_part;

typedef struct {
    ast_node base;
    template_part* parts; // Array of template parts
    size_t part_count; // Number of parts
} ast_template_literal;

typedef struct {
    ast_node base;
    int value; // 0 for false, 1 for true
} ast_boolean;

typedef struct {
    ast_node base;
    // No additional data needed for null
} ast_null;

typedef struct {
    ast_node base;
    // No additional data needed for undefined
} ast_undefined;

typedef struct {
    ast_node base;
    char* name; // Variable/function name
} ast_identifier;

typedef struct {
    ast_node base;
    ast_node** elements; // Array of AST nodes
    size_t count;
} ast_array;

// Binary operation node
typedef struct {
    ast_node base;
    binary_operator op;
    ast_node* left;
    ast_node* right;
} ast_binary_op;

// Ternary conditional node (condition ? true_expr : false_expr)
typedef struct {
    ast_node base;
    ast_node* condition;
    ast_node* true_expr;
    ast_node* false_expr;
} ast_ternary;

// Range expression node
typedef struct {
    ast_node base;
    ast_node* start; // Starting value
    ast_node* end; // Ending value
    int exclusive; // 1 for ..< (exclusive), 0 for .. (inclusive)
} ast_range;

// Unary operation node
typedef struct {
    ast_node base;
    unary_operator op;
    ast_node* operand;
} ast_unary_op;

// Function definition node
typedef struct {
    ast_node base;
    char** parameters; // Parameter names
    size_t param_count;
    ast_node* body; // Block statement or expression
    int is_expression; // 1 if body is expression, 0 if block
} ast_function;

// Function call node
typedef struct {
    ast_node base;
    ast_node* function; // Function expression
    ast_node** arguments; // Argument expressions
    size_t arg_count;
} ast_call;

// Member access node (obj.prop)
typedef struct {
    ast_node base;
    ast_node* object;
    char* property;
} ast_member;

// Object literal property
typedef struct {
    char* key;
    ast_node* value;
} object_property;

// Object literal node
typedef struct {
    ast_node base;
    object_property* properties;
    size_t property_count;
} ast_object_literal;

// Variable declaration node
typedef struct {
    ast_node base;
    char* name;
    ast_node* initializer; // May be NULL
    int is_immutable; // 1 for val, 0 for var
} ast_var_declaration;

// Assignment node
typedef struct {
    ast_node base;
    ast_node* target; // Identifier, member access, or index access
    ast_node* value;
} ast_assignment;

// Compound assignment node
typedef struct {
    ast_node base;
    ast_node* target; // Identifier, member access, or index access
    ast_node* value; // Right-hand side expression
    binary_operator op; // The operation to perform (ADD, SUBTRACT, etc.)
} ast_compound_assignment;

// If statement node
typedef struct {
    ast_node base;
    ast_node* condition;
    ast_node* then_stmt;
    ast_node* else_stmt; // May be NULL
} ast_if;

// While loop node
typedef struct {
    ast_node base;
    ast_node* condition;
    ast_node* body;
} ast_while;

// For loop node
typedef struct {
    ast_node base;
    ast_node* initializer; // Variable declaration or assignment (optional)
    ast_node* condition; // Loop condition (optional, defaults to true)
    ast_node* increment; // Increment expression (optional)
    ast_node* body; // Loop body
} ast_for;

// Do-while loop node
typedef struct {
    ast_node base;
    ast_node* body;
    ast_node* condition;
} ast_do_while;

// Infinite loop node
typedef struct {
    ast_node base;
    ast_node* body;
} ast_loop;

// Break statement node
typedef struct {
    ast_node base;
    // No additional fields needed - break is just a simple statement
} ast_break;

// Continue statement node
typedef struct {
    ast_node base;
    // No additional fields needed - continue is just a simple statement
} ast_continue;

// Return statement node
typedef struct {
    ast_node base;
    ast_node* value; // May be NULL
} ast_return;

// Expression statement node
typedef struct {
    ast_node base;
    ast_node* expression;
} ast_expression_stmt;

// Block statement node
typedef struct {
    ast_node base;
    ast_node** statements;
    size_t statement_count;
} ast_block;

// Program node (root)
typedef struct {
    ast_node base;
    ast_node** statements;
    size_t statement_count;
} ast_program;

// AST creation functions
ast_integer* ast_create_integer(int32_t value, int line, int column);
ast_bigint* ast_create_bigint(di_int value, int line, int column);
ast_number* ast_create_float32(float value, int line, int column);
ast_number* ast_create_float64(double value, int line, int column);
ast_number* ast_create_number(double value, int line, int column);
ast_string* ast_create_string(const char* value, int line, int column);
ast_template_literal* ast_create_template_literal(template_part* parts, size_t part_count, int line, int column);
ast_boolean* ast_create_boolean(int value, int line, int column);
ast_null* ast_create_null(int line, int column);
ast_undefined* ast_create_undefined(int line, int column);
ast_identifier* ast_create_identifier(const char* name, int line, int column);
ast_array* ast_create_array(ast_node** elements, size_t count, int line, int column);

ast_binary_op* ast_create_binary_op(binary_operator op, ast_node* left, ast_node* right, int line, int column);
ast_ternary* ast_create_ternary(ast_node* condition, ast_node* true_expr, ast_node* false_expr, int line, int column);
ast_range* ast_create_range(ast_node* start, ast_node* end, int exclusive, int line, int column);
ast_unary_op* ast_create_unary_op(unary_operator op, ast_node* operand, int line, int column);

ast_function* ast_create_function(char** parameters, size_t param_count, ast_node* body, int is_expression, int line,
                                  int column);
ast_call* ast_create_call(ast_node* function, ast_node** arguments, size_t arg_count, int line, int column);

ast_member* ast_create_member(ast_node* object, const char* property, int line, int column);
ast_object_literal* ast_create_object_literal(object_property* properties, size_t property_count, int line, int column);

ast_var_declaration* ast_create_var_declaration(const char* name, ast_node* initializer, int is_immutable, int line, int column);
ast_assignment* ast_create_assignment(ast_node* target, ast_node* value, int line, int column);
ast_compound_assignment* ast_create_compound_assignment(ast_node* target, ast_node* value, binary_operator op, int line,
                                                        int column);

ast_if* ast_create_if(ast_node* condition, ast_node* then_stmt, ast_node* else_stmt, int line, int column);
ast_while* ast_create_while(ast_node* condition, ast_node* body, int line, int column);
ast_for* ast_create_for(ast_node* initializer, ast_node* condition, ast_node* increment, ast_node* body, int line,
                        int column);
ast_do_while* ast_create_do_while(ast_node* body, ast_node* condition, int line, int column);
ast_loop* ast_create_loop(ast_node* body, int line, int column);
ast_break* ast_create_break(int line, int column);
ast_continue* ast_create_continue(int line, int column);
ast_return* ast_create_return(ast_node* value, int line, int column);

ast_expression_stmt* ast_create_expression_stmt(ast_node* expression, int line, int column);
ast_block* ast_create_block(ast_node** statements, size_t statement_count, int line, int column);
ast_program* ast_create_program(ast_node** statements, size_t statement_count, int line, int column);

// AST utility functions
void ast_free(ast_node* node);
void ast_print(ast_node* node, int indent);
const char* ast_node_type_name(ast_node_type type);

#endif // AST_H
