\ Utility functions module
\ General purpose helper functions

\ Clamp a value between min and max
def clamp(value, min, max) =
    if value < min then
        min
    else if value > max then
        max
    else
        value

\ Linear interpolation
def lerp(start, finish, t) = start + (finish - start) * t

\ Map a value from one range to another
def mapRange(value, inMin, inMax, outMin, outMax) =
    (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin

\ Check if value is within range (inclusive)
def inRange(value, min, max) = value >= min and value <= max

\ Swap two values (returns array)
def swap(a, b) = [b, a]

\ Identity function
def identity(x) = x

\ Constant function generator
def constant(value) = x -> value

\ Compose two functions
def compose(f, g) = x -> f(g(x))

\ Pipe value through functions
def pipe(value, f1, f2) = f2(f1(value))