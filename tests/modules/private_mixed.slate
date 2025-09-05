\ Module with mixed private and public declarations
\ Tests visibility control with private modifier

\ Public val declarations
val PUBLIC_CONST = "visible to importers"
val SHARED_VALUE = 42

\ Public var declarations  
var public_counter = 0
var shared_state = "ready"

\ Public function declarations
def public_function(x) = x * 2
def get_shared_value() = SHARED_VALUE

\ Public data type declarations
data PublicResult
    case Success(value)
    case Failure(error)

data PublicPair(first, second)

\ Private data type declarations  
private data InternalState(value, timestamp)
private data PrivateConfig
    case Development  
    case Production
    case Testing