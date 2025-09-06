\ Mathematical constants module
\ Part of the math package

package examples.modules.math

\ Mathematical constants
val PI = 3.141592653589793
val E = 2.718281828459045
val PHI = 1.618033988749895  \ Golden ratio
val SQRT2 = 1.4142135623730951
val SQRT3 = 1.7320508075688772
val LN2 = 0.6931471805599453
val LN10 = 2.302585092994046

\ Angle conversion constants
val DEG_TO_RAD = PI / 180
val RAD_TO_DEG = 180 / PI

\ Helper functions for angle conversion
def toRadians(degrees) = degrees * DEG_TO_RAD
def toDegrees(radians) = radians * RAD_TO_DEG