// Demo file showing module imports
// This demonstrates various import styles

// Import specific functions from the math package
import examples.modules.math.{add, multiply, PI}

// Import everything from utils
import examples.modules.utils._

// Import with renaming
import examples.modules.math.{factorial => fact, isPrime => prime}

// Main program
print("Module System Demo")
print("==================")

// Using imported math functions
print("Basic math:")
print("  add(5, 3) = " + add(5, 3))
print("  multiply(4, 7) = " + multiply(4, 7))
print("  PI = " + PI)

// Using renamed imports
print("\nAdvanced math:")
print("  factorial(5) = " + fact(5))
print("  isPrime(17) = " + prime(17))

// Using utils functions
print("\nUtility functions:")
print("  clamp(15, 0, 10) = " + clamp(15, 0, 10))
print("  lerp(0, 100, 0.25) = " + lerp(0, 100, 0.25))
print("  inRange(5, 1, 10) = " + inRange(5, 1, 10))

// Function composition
var double = x -> x * 2
var addOne = x -> x + 1
var doubleThenAddOne = compose(addOne, double)
print("  compose(addOne, double)(5) = " + doubleThenAddOne(5))

print("\nModule demo complete!")