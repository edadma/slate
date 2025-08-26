\ Interactive Calculator with User Input
\ Demonstrates input(), parse_number(), and mathematical operations

print("=== Interactive Slate Calculator ===")
print("Enter mathematical expressions or type 'quit' to exit")
print("")

var running = true

while running do
    var user_input = input("calc> ")
    
    if user_input == "quit" or user_input == "exit" then
        running = false
        print("Goodbye!")
    else if user_input == "help" then
        print("Available operations:")
        print("  Basic: +, -, *, /, %, ** (power), // (floor division)")
        print("  Functions: abs(), sqrt(), sin(), cos(), tan(), floor(), ceil(), round()")
        print("  Example: sqrt(16) + 3 * 2")
        print("  Type 'quit' to exit")
    else if user_input == "" then
        \ Skip empty input
        print("")
    else
        \ Try to evaluate the expression as Slate code
        print("Evaluating: " + user_input)
        
        \ For now, we'll show a simple number parsing example
        \ since we can't dynamically execute code yet
        var simple_result = parse_number(user_input)
        if simple_result != null then
            print("Parsed number: " + simple_result)
        else
            print("Complex expression - in a full implementation, this would be evaluated")
            print("For now, try entering simple numbers like: 42, 3.14, etc.")