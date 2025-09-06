\ Module focused on def (function) declarations
\ Tests various function patterns and signatures

\ Simple arithmetic functions
def add(a, b) = a + b
def multiply(a, b) = a * b
def subtract(a, b) = a - b
def divide(a, b) = a / b
def modulo(a, b) = a % b

\ Higher-order functions
def compose(f, g) = x -> f(g(x))
def apply_twice(f) = x -> f(f(x))
def constant_func(value) = x -> value

\ Conditional functions
def maximum(a, b) = if a > b then a else b
def minimum(a, b) = if a < b then a else b
def absolute(n) = if n < 0 then -n else n

\ String functions
def concat_with_space(a, b) = a + " " + b
def repeat_string(str, count) =
    if count <= 0 then
        ""
    else
        str + repeat_string(str, count - 1)

\ Array functions
def first(arr) = arr(0)
def length(arr) = arr.length()
def is_empty(arr) = arr.length() == 0

\ Boolean functions
def not_func(x) = not x
def both(a, b) = a and b
def either(a, b) = a or b