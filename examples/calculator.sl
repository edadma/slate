\ A simple calculator demonstration
\ Shows off Slate's arithmetic operators and built-in math functions

print("=== Slate Calculator ===")

\ Basic arithmetic
var a = 15
var b = 4
print("a = " + a + ", b = " + b)
print("Addition: " + a + " + " + b + " = " + (a + b))
print("Subtraction: " + a + " - " + b + " = " + (a - b))
print("Multiplication: " + a + " * " + b + " = " + (a * b))
print("Division: " + a + " / " + b + " = " + (a / b))
print("Modulo: " + a + " % " + b + " = " + (a % b))
print("Power: " + a + " ** 2 = " + (a ** 2))
print("Floor division: " + a + " // " + b + " = " + (a // b))

print("")

\ Compound assignments
var x = 10
print("Starting with x = " + x)
x += 5
print("After x += 5: " + x)
x *= 2
print("After x *= 2: " + x)
x //= 3
print("After x //= 3: " + x)

print("")

\ Built-in math functions
var num = 16.7
print("Working with " + num + ":")
print("abs(-" + num + ") = " + abs(-num))
print("sqrt(" + num + ") = " + sqrt(num))
print("floor(" + num + ") = " + floor(num))
print("ceil(" + num + ") = " + ceil(num))
print("round(" + num + ") = " + round(num))

print("")
print("min(10, 3) = " + min(10, 3))
print("max(10, 3) = " + max(10, 3))
print("Random number: " + random())

\ Trigonometry
var angle = 1.57  \ approximately pi/2
print("")
print("Trigonometry with " + angle + " radians:")
print("sin(" + angle + ") = " + sin(angle))
print("cos(" + angle + ") = " + cos(angle))
print("tan(" + angle + ") = " + tan(angle))