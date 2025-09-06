\ Control flow examples: if/else and while loops

print("=== Control Flow Demo ===")

\ Simple if/else
var temperature = 75
print("Temperature: " + temperature + "°F")

if temperature > 80 then
    print("It's hot outside!")
else if temperature > 65 then
    print("Nice weather!")
else
    print("It's a bit chilly")

print("")

\ Conditional expressions
var age = 17
var status = if age >= 18 then "adult" else "minor"
print("Age " + age + " is classified as: " + status)

\ Multiple conditions
var score = 85
var grade = if score >= 90 then "A"
           else if score >= 80 then "B"
           else if score >= 70 then "C"
           else if score >= 60 then "D"
           else "F"
print("Score " + score + " gets grade: " + grade)

print("")

\ While loops for counting
print("Counting up:")
var i = 1
while i <= 5 do
    print("Count: " + i)
    i = i + 1

print("")

\ While loop with more complex condition
print("Countdown:")
var countdown = 10
while countdown > 0 do
    if countdown <= 3 then
        print("Almost there... " + countdown)
    else
        print("Countdown: " + countdown)
    countdown = countdown - 1
print("Blast off! 🚀")

print("")

\ Using while for array processing
var numbers = [2, 4, 6, 8, 10]
var sum = 0
var index = 0

print("Calculating sum of " + numbers + ":")
while index < numbers.length do
    var current = numbers(index)
    print("Adding " + current + " (sum so far: " + sum + ")")
    sum = sum + current
    index = index + 1

print("Total sum: " + sum)

print("")

\ Finding maximum in array
var values = [23, 45, 12, 78, 34, 89, 56]
var max_val = values(0)
var pos = 1

print("Finding maximum in " + values + ":")
while pos < values.length do
    if values(pos) > max_val then
        max_val = values(pos)
    pos = pos + 1

print("Maximum value: " + max_val)