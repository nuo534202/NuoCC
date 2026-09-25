# Supported Features

1. `char`, `int` and `long` variable Declaration and Assignment,
   including a pointer to any of them, written `char *b;`. A `char` holds
   0 to 255, an `int` four bytes and a `long` eight. A narrower value is
   widened where the types are mixed; a value which would have to be
   narrowed to fit its target is refused.
2. Arithmetic Operations: Addition, Subtraction, Multiplication, Division.
3. Comparison Operations: Equal To, Not Equal To, Less Than, Greater Than,
   Less Than Or Equal To, Greater Than Or Equal To.
4. Taking the Address of a variable with `&`, and reading through a
   pointer with `*`. Only the `&` of a variable and the `*` of a pointer
   are accepted, and a pointer to a pointer has no type of its own yet.
4. Simple `print` statement.
5. If-Else Statements.
6. While Loops.
7. For Loops.
8. Function Declarations and Calls. A function takes no parameters and
   returns `void`, `char`, `int` or `long`, so at most one argument may be
   passed and it is not yet visible to the function. A function which
   returns a value must end with a `return` statement, and a void function
   cannot return one.
