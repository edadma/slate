# Slate Language Grammar

## Overview
Slate is a dynamically-typed programming language with expression-oriented syntax. This document describes the formal grammar and syntax rules.

## Lexical Elements

### Comments
```
comment        ::= '\' [^\r\n]*
```

### Literals
```
number         ::= integer | float | hexadecimal
integer        ::= [0-9]+
float          ::= [0-9]+ '.' [0-9]+
hexadecimal    ::= '0x' [0-9a-fA-F]+
string         ::= '"' ([^"\\] | '\\' .)* '"'
                 | "'" ([^'\\] | '\\' .)* "'"
boolean        ::= 'true' | 'false'
null           ::= 'null'
undefined      ::= 'undefined'
```

### Identifiers
```
identifier     ::= [a-zA-Z_][a-zA-Z0-9_]*
```

### Keywords
```
keywords       ::= 'var' | 'const' | 'def' | 'if' | 'then' | 'elif' | 'else'
                 | 'while' | 'do' | 'for' | 'loop' | 'break' | 'continue'
                 | 'return' | 'import' | 'match' | 'case' | 'default'
                 | 'true' | 'false' | 'null' | 'undefined' | 'end'
```

## Expressions

### Primary Expressions
```
primary        ::= identifier
                 | literal
                 | '(' expression ')'
                 | array_literal
                 | object_literal
                 | function_expression
```

### Literals
```
array_literal  ::= '[' (expression (',' expression)*)? ']'
object_literal ::= '{' (property (',' property)*)? '}'
property       ::= identifier ':' expression
                 | string ':' expression
```

### Function Expressions
```
function_expr  ::= 'def' '(' parameters? ')' '=' expression
                 | 'def' '(' parameters? ')' '=' NEWLINE INDENT block DEDENT
parameters     ::= identifier (',' identifier)*
```

### Postfix Expressions
```
postfix        ::= primary postfix_op*
postfix_op     ::= '[' expression ']'                    // array indexing
                 | '.' identifier                        // property access
                 | '(' arguments? ')'                    // function call
                 | '?.' identifier                       // optional chaining
                 | '?[' expression ']'                   // optional indexing
arguments      ::= expression (',' expression)*
```

### Unary Expressions
```
unary          ::= postfix
                 | unary_op unary
unary_op       ::= '+' | '-' | '!' | '~' | '++' | '--'
```

### Binary Expressions (by precedence, highest to lowest)
```
multiplicative ::= unary (('*' | '/' | '//' | '%') unary)*
additive       ::= multiplicative (('+' | '-') multiplicative)*
shift          ::= additive (('<<' | '>>' | '>>>') additive)*
bitwise_and    ::= shift ('&' shift)*
bitwise_xor    ::= bitwise_and ('^' bitwise_and)*
bitwise_or     ::= bitwise_xor ('|' bitwise_xor)*
relational     ::= bitwise_or (('<' | '<=' | '>' | '>=' | 'in' | 'instanceof') bitwise_or)*
equality       ::= relational (('==' | '!=') relational)*
logical_and    ::= equality ('&&' equality)*
logical_or     ::= logical_and ('||' logical_and)*
null_coalesce  ::= logical_or ('??' logical_or)*
```

### Assignment Expressions
```
assignment     ::= null_coalesce assignment_op null_coalesce
                 | null_coalesce
assignment_op  ::= '=' | '+=' | '-=' | '*=' | '/=' | '//=' | '%='
                 | '<<=' | '>>=' | '>>>=' | '&=' | '^=' | '|='
                 | '&&=' | '||=' | '??='
```

### Conditional Expressions
```
conditional    ::= assignment ('?' assignment ':' assignment)?
```

### Expression
```
expression     ::= conditional
```

## Statements

### Variable Declarations
```
var_decl       ::= 'var' identifier ('=' expression)?
const_decl     ::= 'const' identifier '=' expression
```

### Function Declarations
```
function_decl  ::= 'def' identifier '(' parameters? ')' '=' expression
                 | 'def' identifier '(' parameters? ')' '=' NEWLINE INDENT block DEDENT
```

### Control Flow
```
if_stmt        ::= 'if' expression 'then' statement_or_block
                   ('elif' expression 'then' statement_or_block)*
                   ('else' statement_or_block)?

while_stmt     ::= 'while' expression ('do' statement_or_block | NEWLINE INDENT block DEDENT)

do_while_stmt  ::= 'do' statement_or_block 'while' expression

for_stmt       ::= 'for' (var_decl | expression)? ';' expression? ';' expression?
                   ('do' statement_or_block | NEWLINE INDENT block DEDENT)

loop_stmt      ::= 'loop' (statement_or_block | NEWLINE INDENT block DEDENT)

break_stmt     ::= 'break'
continue_stmt  ::= 'continue'
return_stmt    ::= 'return' expression?
```

### Match Statements
```
match_stmt     ::= 'match' expression NEWLINE INDENT
                   match_arm+
                   (default_arm)?
                   DEDENT

match_arm      ::= 'case' pattern ':' statement_or_block
default_arm    ::= 'default' ':' statement_or_block
pattern        ::= expression  // Pattern matching syntax
```

### Import Statements
```
import_stmt    ::= 'import' module_path ('.' '{' import_list '}')?
module_path    ::= identifier ('.' identifier)*
import_list    ::= identifier (',' identifier)*
```

### Statement Types
```
statement      ::= var_decl
                 | const_decl  
                 | function_decl
                 | if_stmt
                 | while_stmt
                 | do_while_stmt
                 | for_stmt
                 | loop_stmt
                 | break_stmt
                 | continue_stmt
                 | return_stmt
                 | match_stmt
                 | import_stmt
                 | expression_stmt

expression_stmt ::= expression

statement_or_block ::= statement
                     | NEWLINE INDENT block DEDENT

block          ::= statement*
```

## Language Features

### Expression Functions
Slate supports both single-line and multi-line expression functions:

```slate
\ Single-line expression function
def add(a, b) = a + b

\ Multi-line expression function
def complex_calc(x) =
    var temp = x * 2
    var result = temp + 10
    result  \ Last expression is returned
```

### Block Expressions  
Indented blocks can be used as expressions, returning the value of the last expression:

```slate
var result = 
    var x = 5
    var y = 10
    x + y  \ Returns 15
```

### Assignment Expressions
Assignments return the assigned value, enabling chained assignments and assignments within expressions:

```slate
var a = b = c = 5  \ All variables get value 5
while (i = i - 1) > 0 do print(i)  \ Assignment in condition
```

### Optional Chaining
Safe property access with null/undefined checking:

```slate
obj?.property      \ Returns null if obj is null/undefined
arr?[index]        \ Returns null if arr is null/undefined
```

### Pattern Matching
Match expressions provide sophisticated control flow:

```slate
match value
    case 0: "zero"
    case 1: "one"  
    case x if x > 10: "big number"
    default: "other"
```

## Indentation and Scoping

Slate uses significant whitespace (indentation) to define code blocks:

- **INDENT**: Increase in indentation level starts a new block
- **DEDENT**: Decrease in indentation level ends a block  
- **NEWLINE**: Line breaks are significant for statement separation

### Scope Rules
- Function parameters create local scope
- Variable declarations create local scope within current block
- Loop variables (for loop initializers) create local scope for the loop body
- Blocks create new local scopes
- Variable shadowing is allowed (local variables can shadow outer scope variables)

### Module System
- Each `.sl` file is a module
- Modules execute in shared VM with namespace-based isolation
- Import statements bring module symbols into current namespace
- Selective imports supported: `import module.{symbol1, symbol2}`

## Operator Precedence (highest to lowest)

1. **Postfix**: `[]` `.` `()` `?.` `?[]` `++` `--`
2. **Unary**: `+` `-` `!` `~` `++` `--` (prefix)
3. **Power**: `**` (right-associative)
4. **Multiplicative**: `*` `/` `//` `%`
5. **Additive**: `+` `-`
6. **Shift**: `<<` `>>` `>>>`
7. **Bitwise AND**: `&`
8. **Bitwise XOR**: `^`
9. **Bitwise OR**: `|`
10. **Relational**: `<` `<=` `>` `>=` `in` `instanceof`
11. **Equality**: `==` `!=`
12. **Logical AND**: `&&`
13. **Logical OR**: `||`
14. **Null Coalescing**: `??`
15. **Assignment**: `=` `+=` `-=` `*=` `/=` `//=` `%=` `<<=` `>>=` `>>>=` `&=` `^=` `|=` `&&=` `||=` `??=`

## Built-in Functions

### Type Functions
- `type(value)` - Returns type of value as string
- `abs(number)` - Absolute value
- `sqrt(number)` - Square root
- `floor(number)` - Floor function
- `ceil(number)` - Ceiling function

### I/O Functions  
- `print(...)` - Print values to stdout
- `input(prompt?)` - Read line from stdin

### Collection Functions
- `Array(...)` - Array constructor
- `String(...)` - String constructor
- `Buffer(...)` - Buffer constructor

### Math Functions
- `sin(x)`, `cos(x)`, `tan(x)` - Trigonometric functions
- `asin(x)`, `acos(x)`, `atan(x)`, `atan2(y,x)` - Inverse trig functions
- `exp(x)`, `ln(x)` - Exponential and natural logarithm
- `min(a,b)`, `max(a,b)` - Minimum and maximum
- `random()` - Random number 0-1

## Type System

Slate is dynamically typed with these built-in types:

- **Number**: `int32`, `bigint`, `float`
- **String**: UTF-8 encoded strings
- **Boolean**: `true`, `false`
- **Null**: `null`
- **Undefined**: `undefined` 
- **Array**: Dynamic arrays `[1, 2, 3]`
- **Object**: Key-value maps `{key: value}`
- **Function**: First-class functions
- **Range**: Numeric ranges `1..10`, `1...10` (exclusive)
- **Buffer**: Byte buffers for binary data
- **Iterator**: Lazy iteration over collections

### Type Coercion
- Arithmetic operations promote int32 to float when needed
- Integer overflow promotes int32 to bigint
- Division always produces float: `5 / 2 → 2.5`
- Floor division produces integer: `5 // 2 → 2`
- String concatenation coerces operands to strings

This grammar specification covers the current implementation of Slate as of the latest version.