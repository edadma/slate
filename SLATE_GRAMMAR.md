# Slate Language Grammar

## Overview
Slate is a dynamically-typed programming language with expression-oriented syntax. Control flow constructs are expressions that return values, while declarations and imports are statements that do not return values.

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
keywords       ::= 'var' | 'val' | 'def' | 'if' | 'then' | 'elif' | 'else'
                 | 'while' | 'do' | 'for' | 'loop' | 'break' | 'continue'
                 | 'return' | 'import' | 'package' | 'private' | 'match' | 'case' | 'default' | 'data'
                 | 'true' | 'false' | 'null' | 'undefined' | 'NaN' | 'Infinity'
                 | 'and' | 'or' | 'not' | 'in' | 'instanceof' | 'mod' | 'step' | 'end'
```

## Expressions

### Primary Expressions
```
primary        ::= identifier
                 | number
                 | string
                 | template_literal
                 | boolean
                 | null
                 | undefined
                 | NaN
                 | Infinity
                 | '(' expression ')'
                 | array_literal
                 | object_literal
                 | indented_block
                 | anonymous_function
                 | if_expression
                 | while_expression
                 | do_while_expression
                 | for_expression
                 | loop_expression
                 | match_expression
                 | break_expression
                 | continue_expression
                 | return_expression
```

### Literals
```
array_literal     ::= '[' (expression (',' expression)*)? ']'
object_literal    ::= '{' (property (',' property)*)? '}'
property          ::= identifier ':' expression
                    | string ':' expression
template_literal  ::= '`' template_part* '`'
template_part     ::= template_text
                    | '$' identifier
                    | '${' expression '}'
```

### Anonymous Functions
```
anonymous_function ::= identifier '->' expression                  // single parameter
                     | '(' ')' '->' expression                    // no parameters
                     | '(' parameters ')' '->' expression         // multiple parameters
```

### Control Flow Expressions
```
if_expression  ::= 'if' expression 'then' expression elif_part* else_part?
                 | 'if' expression ['then'] indented_block elif_part* else_part? ['end' 'if']

elif_part      ::= 'elif' expression 'then' expression
                 | 'elif' expression ['then'] indented_block

else_part      ::= 'else' expression

while_expression ::= 'while' expression 'do' expression
                   | 'while' expression indented_block ['end' 'while']

do_while_expression ::= 'do' expression 'while' expression

for_expression ::= 'for' (var_decl | expression)? ';' expression? ';' expression?
                   ('do' expression | indented_block ['end' 'for'])

loop_expression ::= 'loop' expression
                  | 'loop' indented_block ['end' 'loop']

match_expression ::= 'match' expression indented_match_block ['end' 'match']

indented_match_block ::= NEWLINE INDENT
                        match_arm+
                        (default_arm)?
                        DEDENT

match_arm      ::= 'case' pattern 'do' expression
                 | 'case' pattern expression
default_arm    ::= 'default' expression
pattern        ::= expression
```

### Special Expressions (parsed as statements but usable in expression contexts)
```
break_expression    ::= 'break'
continue_expression ::= 'continue'  
return_expression   ::= 'return' expression?
```

### Postfix Expressions
```
postfix        ::= primary postfix_op*
postfix_op     ::= '(' arguments? ')'                     // function call or array access
                 | '.' identifier                        // property access
                 | '?.' identifier                       // optional chaining
arguments      ::= expression (',' expression)*
```

### Unary Expressions
```
unary          ::= postfix
                 | unary_op unary
unary_op       ::= '+' | '-' | '!' | 'not' | '~' | '++' | '--'
```

### Binary Expressions (by precedence, highest to lowest)
```
power          ::= unary ('**' unary)*                   // right-associative
multiplicative ::= power (('*' | '/' | '//' | '%' | 'mod') power)*
additive       ::= multiplicative (('+' | '-') multiplicative)*
shift          ::= additive (('<<' | '>>' | '>>>') additive)*
range          ::= shift (('..' | '..<') shift ['step' shift])?
bitwise_and    ::= range ('&' range)*
bitwise_xor    ::= bitwise_and ('^' bitwise_and)*
bitwise_or     ::= bitwise_xor ('|' bitwise_xor)*
relational     ::= bitwise_or (('<' | '<=' | '>' | '>=' | 'in' | 'instanceof') bitwise_or)*
equality       ::= relational (('==' | '!=') relational)*
logical_and    ::= equality (('&&' | 'and') equality)*
logical_or     ::= logical_and (('||' | 'or') logical_and)*
null_coalesce  ::= logical_or ('??' logical_or)*
```

### Assignment Expressions
```
assignment     ::= ternary assignment_op assignment
                 | ternary
assignment_op  ::= '=' | '+=' | '-=' | '*=' | '/=' | '//=' | '%=' | '**='
                 | '<<=' | '>>=' | '>>>=' | '&=' | '^=' | '|='
                 | '&&=' | '||=' | '??='
```

### Ternary Conditional
```
ternary        ::= null_coalesce ('?' assignment ':' assignment)?
```

### Expression Definition
```
expression     ::= assignment
                 | anonymous_function
                 | indented_block
```

## Statements

Statements are language constructs that do not return values. They are used for declarations, imports, and other side effects.

### Variable Declarations
```
var_decl       ::= 'var' identifier ('=' expression)?
val_decl       ::= 'val' identifier '=' expression
```

### Function Declarations
```
function_decl  ::= 'def' identifier '(' parameters? ')' '=' expression
                 | 'def' identifier '(' parameters? ')' '=' indented_block ['end' identifier]
parameters     ::= identifier (',' identifier)*
```

### Import Statements
```
import_stmt    ::= 'import' module_path '.' '_'                    // wildcard import
                 | 'import' module_path '.' '{' import_list '}'    // selective import
                 | 'import' module_path '.' identifier             // single item import
                 | 'import' module_path                            // namespace import

module_path    ::= identifier ('.' identifier)*
import_list    ::= import_spec (',' import_spec)*
import_spec    ::= identifier ('=>' identifier)?                   // with optional alias
```

### Package Statements
```
package_stmt   ::= 'package' package_path
package_path   ::= identifier ('.' identifier)*
```

### Data Type Declarations
```
data_decl      ::= ['private'] 'data' identifier ['(' parameters ')']
                   (NEWLINE INDENT data_constructor* DEDENT)?
                   ['end' identifier]

data_constructor ::= 'case' identifier ['(' parameters ')']
```

### Statement Types
```
statement      ::= var_decl
                 | val_decl  
                 | function_decl
                 | import_stmt
                 | package_stmt
                 | data_decl
                 | expression_stmt

expression_stmt ::= expression
```

### Block and Program Structure
```
indented_block ::= NEWLINE INDENT block DEDENT

block          ::= (statement | expression)*

program        ::= block
```

## End Markers

Slate supports optional end markers for constructs that use indented blocks. End markers provide visual clarity and help with error checking, especially in large blocks.

### End Marker Rules

**End markers are only available when using indented blocks:**
- Single-line forms (using `do`, `then`, or direct expressions) do not support end markers
- Multi-line indented block forms support optional end markers

**Supported end markers:**
- **Control flow**: `end if`, `end while`, `end for`, `end loop`, `end match`
- **Function declarations**: `end <function_name>` (where `<function_name>` is the actual function name)  
- **Data declarations**: `end <DataTypeName>` (where `<DataTypeName>` is the actual data type name)

### Examples

```slate
\ Control flow with end markers
if condition
    do_something()
    result = compute()
    result
end if

while condition
    process_item()
    update_condition()
end while

for var i = 0; i < 10; i += 1
    print(i)
end for

loop
    if should_exit() then break
    process()
end loop

match value
    case 1 do "one"
    case 2 do "two"
    default "other"
end match

\ Function with end marker
def fibonacci(n) =
    if n <= 1 then
        return n
    fibonacci(n-1) + fibonacci(n-2)
end fibonacci

\ Data declaration with end marker
data BinaryTree(value)
    case Leaf
    case Node(left, right)
        
    def size() =
        match this
            case Leaf do 0
            case Node(l, r) do 1 + l.size() + r.size()
end BinaryTree
```

**Note:** `do...while` loops are self-delimiting and do not support end markers since they end with the `while` keyword.

## Language Semantics

### Expression Return Values

**Control Flow Expressions:**
- **if expression**: Returns value of executed branch. If condition is false and no `else`, returns `undefined`
- **while/do-while/for/loop expressions**: Return `undefined`
- **match expression**: Returns value of matched case or default
- **break/continue expressions**: Return `undefined` (but cause control flow change)
- **return expression**: Returns the specified value (or `undefined` if no value given)

**Other Expressions:**
- **Assignment expressions**: Return the assigned value
- **Arithmetic/logical expressions**: Return computed result
- **Function calls**: Return function result
- **Literals**: Return their literal value

### Statement vs Expression Distinction

**Statements** (do not return values):
- Variable declarations (`var`, `val`)
- Function declarations (`def`)  
- Import statements (`import`)
- Expression statements (expressions used as statements)

**Expressions** (return values):
- All control flow constructs (`if`, `while`, `for`, `loop`, `match`)
- Assignments, arithmetic, function calls, literals
- `break`, `continue`, `return` (parsed as statements but usable in expression contexts)

### Special Handling of break/continue/return

These are parsed as statements to allow them at the end of blocks (which are expressions), but they can appear in expression contexts. The parser/codegen handles them specially:

- They can appear as the final element in a block expression
- When used in expression context, they evaluate to `undefined` before causing control flow change
- They require special codegen handling due to their dual nature

### Block Expressions (Indented Blocks)

**Indented blocks are first-class expressions** in Slate. They can appear anywhere an expression is expected and always return a value.

#### Syntax
```
indented_block ::= NEWLINE INDENT block DEDENT
block          ::= (statement | expression)*
```

#### Block Expression Semantics
An indented block is created by:
- A newline followed by increased indentation (INDENT)
- A sequence of statements and/or expressions  
- A dedent back to the previous indentation level (DEDENT)

The **return value** of an indented block is:
1. The value of the last expression in the block, OR
2. `undefined` if the block ends with a statement, OR  
3. `undefined` if the block is empty

#### Usage as Expressions
Since indented blocks are expressions, they can be used:
- As the right-hand side of assignments (`var x = indented_block`)
- As function arguments (`print(indented_block)`)
- As operands in binary operations (`indented_block + 5`)
- Anywhere else an expression is valid

#### Examples
```slate
\ Assignment to variable
val result = 
    var x = 5      \ Statement - doesn't contribute to block value
    x + 10         \ Expression - block evaluates to 15
print(result)      \ Prints: 15

\ As function argument
print(
    var temp = 100
    temp / 2
)                  \ Prints: 50

\ In arithmetic operations
val sum = 10 + 
    var a = 5
    var b = 3
    a * b          \ Block evaluates to 15
print(sum)         \ Prints: 25

\ Nested blocks
val nested =
    val outer = 10
    
        val inner = 5
        outer + inner     \ Inner block evaluates to 15
print(nested)      \ Prints: 15
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

**Import Forms:**
```slate
import math._                       \ Wildcard - imports all exports
import math.{sin, cos => cosine}    \ Selective - imports specific items with optional aliases
import math.sqrt                    \ Single item - imports just sqrt
import math                         \ Namespace - imports as namespace object
```

## Operator Precedence (highest to lowest)

1. **Postfix**: `()` `.` `?.` `++` `--`
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
16. **Ternary Conditional**: `? :`

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

## Examples

### Declarations (Statements)
```slate
\ Variable declarations are statements - they don't return values
var mutable = 5          \ Mutable variable (can be reassigned)
val constant = 10        \ Immutable variable (cannot be reassigned)

\ Function declarations are statements - they don't return values
def add(a, b) = a + b
def complex(x) =
    val temp = x * 2     \ Using val for immutable local variables
    temp + 10
```

### Control Flow as Expressions
```slate
\ if expression returns value of executed branch
var result = if x > 0 then "positive" else "non-positive"

\ 'then' is optional with indented blocks
var result2 = if x > 0
    print("positive!")
    "positive"
else
    "non-positive"

\ if with no else returns undefined when condition is false
var maybe = if false then "value"  \ maybe gets undefined

\ while expression with 'do'
var status = while condition do process_item()

\ 'do' is optional with indented blocks
while i < 10
    print(i)
    i = i + 1

\ for loop - 'do' optional with indented block
for var i = 0; i < 10; i += 1
    print(i)
```

### Block Expressions
```slate
var computed = 
    var a = 5
    var b = 10
    a * b + 2  \ Block returns 52

var sideEffect =
    print("Computing...")
    computeValue()  \ Block returns whatever computeValue() returns
```

### Array Access
```slate
\ Array access uses parentheses (like Scala)
var arr = [1, 2, 3]
print(arr(0))       \ Prints: 1
print(arr(2))       \ Prints: 3
```

### Match Expressions
```slate
\ Match expressions support flexible case syntax
var result = match value
    case 0 do "zero"                    \ Single-line with 'do'
    case 1                              \ Multi-line without 'do'
        "one"
    case x do                           \ Multi-line with 'do'
        if x > 10 then "big" else "small"
    default "other"                     \ Default case (no 'do' needed)

\ Variable binding in patterns
match person
    case john do print("Found John!")   \ Binds the value to 'john'
    default print("Someone else")       \ Default case (no 'do' needed)
```

This grammar specification accurately reflects Slate's expression-oriented design where control flow constructs are expressions that return values, while declarations and imports are statements.