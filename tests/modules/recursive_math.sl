\ Test module for recursive and self-referencing functions
\ Used to test module namespace access bug fix

\ Factorial function (recursive)
def factorial(n) =
    if n <= 1 then
        1
    else
        n * factorial(n - 1)

\ Fibonacci function (recursive)
def fibonacci(n) =
    if n <= 1 then
        n
    else
        fibonacci(n - 1) + fibonacci(n - 2)

\ Greatest common divisor (recursive)
def gcd(a, b) =
    if b == 0 then
        abs(a)
    else
        gcd(b, a % b)

\ Function that calls another function in the same module
def factorial_and_gcd(n, a, b) =
    factorial(n) + gcd(a, b)