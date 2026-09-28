# Supported Features

1. `char`, `int` and `long` variable Declaration and Assignment,
   including a pointer to any of them, written `char *b;`. A `char` holds
   0 to 255, an `int` four bytes and a `long` eight. A narrower value is
   widened where the types are mixed; a value which would have to be
   narrowed to fit its target is refused.
2. Arithmetic Operations: Addition, Subtraction, Multiplication, Division.
3. Comparison Operations: Equal To, Not Equal To, Less Than, Greater Than,
   Less Than Or Equal To, Greater Than Or Equal To.
4. Taking the Address of a variable with `&`, and reading or writing
   through a pointer with `*`. A pointer may be the target of an
   assignment: `*y= 14;` stores into the location `y` points at, and only
   into that location, so writing through a `char` pointer does not run
   over the value stored next to it. Only the `&` of a variable and the
   `*` of a pointer are accepted, and a pointer to a pointer has no type
   of its own yet.
5. Simple `print` statement.
6. If-Else Statements.
7. While Loops.
8. For Loops.
9. Function Declarations and Calls. A function returns `void`, `char`, `int`
   or `long` and takes zero or one parameter. The parameter is visible in the
   function body, and an argument must match its type after widening. A
   function which returns a value must end with a `return` statement, and a
   void function cannot return one.
10. Global Variables. A declaration outside every function declares a
    variable which every function can use. One declaration may name
    several variables of the same type, written `int x, y, z;`, and the
    same is allowed for the variables declared inside a function. The
    `*` of a pointer type belongs to the declaration and not to the name,
    so it is written once: `char *b;` and not `char *b, *c;`. A name has
    to be declared before it is used, so a function can only reach the
    global variables declared above it. The variables are stored one
    after another in the order they are declared.
11. Pointer Arithmetic. An integer may be added to or subtracted from a
    pointer, and is scaled by the size of what the pointer points at, so
    `&c + 1` is the address of the next value of `c`'s type and not the
    address one byte along. Nothing else may be done with a pointer: it
    cannot be multiplied, divided or compared yet, and its address cannot
    be printed.
12. Function-local Variables. Variables declared inside a function are
    stored in that function's stack frame, are released when the function
    returns, and may have the same name as a variable in another function.
13. Function Parameters. A function may bind one typed parameter, such as
    `int add(int value)`, and callers may pass an expression of a compatible
    type. Parameterless functions may be called with empty parentheses.
14. Assignment Expressions. An assignment is an expression which yields the
    value stored, so `x= 5` may be used wherever a value may, as in
    `print x= 5;`. `=` binds less tightly than every other operator and to
    the right, so `c= a= 7` stores into `a` first and then into `c`. What
    may stand on its left is a variable or the location a pointer holds,
    and the value has to fit the target the same way it has to fit a
    variable.
