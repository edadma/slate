\ Circular dependency test module B  
\ This module imports from circular_a to create circular dependency

import circular_a.{value_from_a}

val value_from_b = "B's value"  
var combined_value = value_from_b + " + " + value_from_a

def function_from_b() = "Function B called"