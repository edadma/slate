\ Object manipulation and property access

print("=== Object Playground ===")

\ Create objects
var person = {
    name: "Alice",
    age: 30,
    city: "New York"
}

var book = {
    title: "The Slate Guide",
    author: "Anonymous",
    pages: 150,
    published: true
}

print("Person: " + person)
print("Book: " + book)

print("")

\ Property access using dot notation
print("Person's name: " + person.name)
print("Person's age: " + person.age)
print("Book title: " + book.title)
print("Book pages: " + book.pages)

print("")

\ Property access using function call syntax (dynamic)
var prop = "city"
print("Person's " + prop + ": " + person(prop))

var key = "author"
print("Book " + key + ": " + book(key))

print("")

\ Nested objects
var company = {
    name: "Tech Corp",
    location: {
        street: "123 Main St",
        city: "San Francisco",
        state: "CA"
    },
    employees: 50
}

print("Company: " + company)
print("Company name: " + company.name)
print("Company location: " + company.location)
print("Street: " + company.location.street)
print("Employees: " + company.employees)

print("")

\ Objects with different value types
var showcase = {
    number: 42,
    string: "hello",
    boolean: true,
    null_value: null,
    array: [1, 2, 3],
    nested: {inner: "value"}
}

print("Showcase object: " + showcase)
print("Number property: " + showcase.number)
print("Array property: " + showcase.array)
print("Nested property: " + showcase.nested)
print("Inner value: " + showcase.nested.inner)