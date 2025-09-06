// Advanced math operations module
// Part of the math package

package examples.modules.math

// Power function (using built-in **)
def pow(base, exponent) = base ** exponent

// Factorial function
def factorial(n) =
    if n <= 1 then
        1
    else
        n * factorial(n - 1)

// Greatest common divisor
def gcd(a, b) =
    if b == 0 then
        abs(a)
    else
        gcd(b, a % b)

// Least common multiple
def lcm(a, b) = abs(a * b) / gcd(a, b)

// Check if number is prime
def isPrime(n) =
    if n <= 1 then
        false
    else if n <= 3 then
        true
    else if n % 2 == 0 or n % 3 == 0 then
        false
    else
        var i = 5
        while i * i <= n do
            if n % i == 0 or n % (i + 2) == 0 then
                return false
            i = i + 6
        true