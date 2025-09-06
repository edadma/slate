\ Array operations and demonstrations

print("=== Array Fun ===")

\ Create some arrays
var fruits = ["apple", "banana", "cherry"]
var numbers = [1, 2, 3, 4, 5]
var mixed = [42, "hello", true, null]

print("Fruits: " + fruits)
print("Numbers: " + numbers)
print("Mixed: " + mixed)

print("")

\ Array indexing (using function call syntax)
print("First fruit: " + fruits(0))
print("Second number: " + numbers(1))
print("Mixed array at index 2: " + mixed(2))

print("")

\ Array lengths
print("Fruits length: " + fruits.length())
print("Numbers length: " + numbers.length())

print("")

\ Array concatenation
var more_fruits = ["date", "elderberry"]
var all_fruits = fruits + more_fruits
print("Original fruits: " + fruits)
print("More fruits: " + more_fruits) 
print("All fruits: " + all_fruits)

print("")

\ Nested arrays
var matrix = [[1, 2], [3, 4], [5, 6]]
print("Matrix: " + matrix)
print("First row: " + matrix(0))
print("Element at [1][0]: " + matrix(1)(0))

print("")

\ Empty arrays
var empty = []
print("Empty array: " + empty)
print("Empty length: " + empty.length())
var combined = empty + [1, 2, 3]
print("Empty + [1,2,3]: " + combined)