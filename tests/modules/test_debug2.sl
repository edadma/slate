def add(a, b) = a + b
def multiply(a, b) = a * b

var f = x -> add(x, 5)
var g = x -> multiply(x, 2)

g(3)
