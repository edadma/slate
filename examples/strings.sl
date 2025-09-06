\ String manipulation and operations

print("=== String Playground ===")

\ Basic string operations
var greeting = "Hello"
var name = "Alice"
var full_greeting = greeting + ", " + name + "!"
print(full_greeting)

print("")

\ String with numbers
var age = 25
var message = name + " is " + age + " years old"
print(message)

\ Mixed concatenation
var score = 95.5
var result = "Test score: " + score + "% - " + if score >= 90 then "Excellent!" else "Good work!"
print(result)

print("")

\ String properties and methods
var word = "Programming"
print("Word: " + word)
print("Length: " + word.length())
print("First letter: " + word(0))
print("Last letter: " + word(word.length() - 1))
print("Middle letter: " + word(word.length() // 2))

print("")

\ String method demonstrations
var sample = "  Hello, World!  "
print("Original: '" + sample + "'")
print("Trimmed: '" + sample.trim() + "'")
print("Upper case: " + sample.trim().toUpper())
print("Lower case: " + sample.trim().toLower())

print("")

\ Substring operations
var phrase = "The quick brown fox"
print("Original phrase: " + phrase)
print("First 3 chars: " + phrase.substring(0, 3))
print("Middle word: " + phrase.substring(4, 5))
print("Last word: " + phrase.substring(16, 3))

print("")

\ String search and testing
var text = "Programming is fun and rewarding"
print("Text: " + text)
print("Starts with 'Program': " + text.startsWith("Program"))
print("Ends with 'ing': " + text.endsWith("ing"))
print("Contains 'fun': " + text.contains("fun"))
print("Contains 'boring': " + text.contains("boring"))
print("Index of 'fun': " + text.indexOf("fun"))
print("Index of 'xyz': " + text.indexOf("xyz"))

print("")

\ String replacement
var original = "I love cats and cats love me"
print("Original: " + original)
print("Replace cats with dogs: " + original.replace("cats", "dogs"))
print("Replace 'love' with 'adore': " + original.replace("love", "adore"))

print("")

\ Method chaining examples
var messy_input = "  HELLO world!  "
print("Input: '" + messy_input + "'")
print("Cleaned and formatted: " + messy_input.trim().toLower().replace("hello", "Hi"))

var email = "user@EXAMPLE.COM"
print("Email: " + email)
print("Normalized: " + email.trim().toLower())

print("")

\ Building strings character by character
var letters = ["H", "e", "l", "l", "o"]
var built_word = ""
var i = 0
while i < letters.length() do
    built_word = built_word + letters(i)
    i = i + 1
print("Built word: " + built_word)
print("Built word length: " + built_word.length())

print("")

\ String validation examples
var username = "alice_123"
var email_addr = "user@domain.com"

print("Username validation:")
print("  Username: " + username)
var first_char = username.substring(0, 1)
var is_letter = first_char.toUpper() != first_char.toLower()
print("  Starts with letter: " + is_letter)
print("  Contains underscore: " + username.contains("_"))
var len = username.length()
var len_valid = len >= 6 && len <= 12
print("  Length valid (6-12): " + len_valid)

print("")
print("Email validation:")
print("  Email: " + email_addr)
print("  Contains @: " + email_addr.contains("@"))
print("  Ends with .com: " + email_addr.endsWith(".com"))

print("")

\ String comparison and conditionals
var password = "secret123"
var input = "secret123"

if password == input then
    print("Access granted!")
else
    print("Access denied!")

\ Working with boolean strings
var status = if true then "active" else "inactive"
print("System status: " + status)

\ Advanced string processing
var csv_data = "name:John,age:30,city:Boston"
print("CSV-like data: " + csv_data)
print("Extract name: " + csv_data.substring(5, 4))
print("Find age position: " + csv_data.indexOf("age:"))
var step1 = csv_data.replace(":", " = ")
var formatted = step1.replace(",", " | ")
print("Clean and format: " + formatted)