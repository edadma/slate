\ Greeting program that uses command line arguments
\ Usage: slate -f greeting.sl <name> [age]

print("=== Slate Greeting Program ===")

var arguments = args()

if arguments.length == 0 then
    print("Hello, anonymous user!")
else if arguments.length == 1 then
    print("Hello, " + arguments(0) + "! Nice to meet you.")
else if arguments.length == 2 then
    print("Hello, " + arguments(0) + "!")
    var age = parse_int(arguments(1))
    if age != null then
        print("You're " + age + " years old!")
    else
        print("That doesn't look like a valid age.")
else
    print("Too many arguments!")
    
print("Usage: slate -f greeting.sl <name> [age]")