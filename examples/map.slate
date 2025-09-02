\ Test implementing .map() in Slate instead of C

\ Define a pure Slate implementation of map using iterators
def array_map(arr, func) = 
    var result = []
    var iter = arr.iterator()

    while iter.hasNext() do
        result.push(func(iter.next()))

    result

\ Test the Slate-based map implementation
print("Testing Slate-based map implementation:")

var numbers = [1, 2, 3, 4, 5]
var double = x -> x * 2

print("Original array: " + numbers)
print("Doubling function: x -> x * 2")

var doubled = array_map(numbers, double)
print("Result: " + doubled)

\ Test with more complex function
var addTen = x -> x + 10
var result2 = array_map(numbers, addTen)
print("Add 10 result: " + result2)

\ Test with inline lambda
var result3 = array_map(numbers, x -> x * x)
print("Square result: " + result3)