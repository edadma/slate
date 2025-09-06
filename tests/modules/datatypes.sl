\ Module focused on data (ADT) declarations
\ Tests various algebraic data type patterns

\ Simple singleton cases
data Empty
data Unit

\ Single constructor data types
data Name(first, last)  
data Coordinates(x, y, z)
data Student(name, age, grade)
data Book(title, author, pages)

\ Multi-case algebraic data types
data Color
    case Red
    case Green  
    case Blue
    case RGB(r, g, b)

data Animal
    case Dog(name, breed)
    case Cat(name, color)  
    case Bird(species, can_fly)

data BinaryTree
    case Leaf
    case Node(value, left, right)

data Status
    case Pending
    case InProgress(percent)
    case Completed(timestamp)
    case Failed(error_message)

data Vehicle
    case Car(make, model, year)
    case Truck(capacity, wheels)
    case Motorcycle(engine_cc)
    case Bicycle