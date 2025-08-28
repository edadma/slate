\ Number Converter and Calculator
\ Demonstrates input(), parse_int(), parse_number() functions

print("=== Number Converter ===")
print("Convert strings to numbers and perform calculations")
print("")

\ Get user input for numbers
var num1_str = input("Enter first number: ")
var num2_str = input("Enter second number: ")

\ Parse the numbers
print("")
print("Converting strings to numbers...")

var num1 = parse_number(num1_str)
var num2 = parse_number(num2_str)

if num1 == null then
    print("Error: '" + num1_str + "' is not a valid number")
else if num2 == null then
    print("Error: '" + num2_str + "' is not a valid number")
else
    print("First number: " + num1 + " (type: " + type(num1) + ")")
    print("Second number: " + num2 + " (type: " + type(num2) + ")")
    print("")
    
    \ Perform calculations
    print("=== Calculations ===")
    var sum = num1 + num2
    var diff = num1 - num2
    var product = num1 * num2
    print(num1 + " + " + num2 + " = " + sum)
    print(num1 + " - " + num2 + " = " + diff)
    print(num1 + " * " + num2 + " = " + product)
    
    if num2 != 0 then
        var quotient = num1 / num2
        var floor_div = num1 // num2
        var remainder = num1 % num2
        print(num1 + " / " + num2 + " = " + quotient)
        print(num1 + " // " + num2 + " = " + floor_div)
        print(num1 + " % " + num2 + " = " + remainder)
    else
        print("Cannot divide by zero")
    
    print("")
    
    \ Math functions
    print("=== Math Functions ===")
    var abs1 = abs(num1)
    var abs2 = abs(num2)
    print("abs(" + num1 + ") = " + abs1)
    print("abs(" + num2 + ") = " + abs2)
    
    if num1 >= 0 then
        var sqrt1 = sqrt(num1)
        print("sqrt(" + num1 + ") = " + sqrt1)
    
    if num2 >= 0 then
        var sqrt2 = sqrt(num2)
        print("sqrt(" + num2 + ") = " + sqrt2)
    
    var minimum = min(num1, num2)
    var maximum = max(num1, num2)
    print("min(" + num1 + ", " + num2 + ") = " + minimum)
    print("max(" + num1 + ", " + num2 + ") = " + maximum)

print("")
print("Demo complete!")

\ Also demonstrate integer parsing
print("")
print("=== Integer Parsing Demo ===")
var int_str = input("Enter an integer: ")
var parsed_int = parse_int(int_str)

if parsed_int == null then
    print("Error: '" + int_str + "' is not a valid integer")
else
    print("Parsed integer: " + parsed_int + " (type: " + type(parsed_int) + ")")
    var doubled = parsed_int * 2
    print("Double it: " + doubled)