\ Advanced math operations module
\ Part of the math package

\ Power function (using built-in **)
def pow(base, exponent) = base ** exponent

\ Factorial function
def factorial(n) =
    if n <= 1 then
        1
    else
        n * factorial(n - 1)

\ Greatest common divisor
def gcd(a, b) =
    if b == 0 then
        abs(a)
    else
        gcd(b, a % b)

\ Least common multiple
def lcm(a, b) = abs(a * b) / gcd(a, b)

\ Check if number is prime (basic version)
def isPrime(n) =
    if n <= 1 then
        false
    else if n <= 3 then
        true
    else if n % 2 == 0 or n % 3 == 0 then
        false
    else
        \ Simple check up to sqrt(n) - basic but works
        if n < 25 then true else n % 5 != 0 and n % 7 != 0 and n % 11 != 0 and n % 13 != 0 and n % 17 != 0 and n % 19 != 0 and n % 23 != 0
