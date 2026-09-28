# Supported Features

1. `char`, `int` and `long` variable Declaration and Assignment,
   including a pointer to any of them, written `char *b;`. A `char` holds
   0 to 255, an `int` four bytes and a `long` eight. A narrower value is
   widened where the types are mixed; a value which would have to be
   narrowed to fit its target is refused.
2. Arithmetic Operations: Addition, Subtraction, Multiplication, Division.
   A `-` written before a value negates it; a `char` is widened to an `int`
   first, as an unsigned value has no negative one.
3. Bitwise Operations: And `&`, Or `|`, Xor `^`, Not `~`, Shift Left `<<`
   and Shift Right `>>`. A shift of a negative value fills the top bits in
   with zeroes.
4. Logical Operations: And `&&`, Or `||` and Not `!`. Each answers with 0
   or 1, and `!` is 1 when its value is zero. Neither `&&` nor `||` short
   circuits yet, so both sides are always worked out.
5. Comparison Operations: Equal To, Not Equal To, Less Than, Greater Than,
   Less Than Or Equal To, Greater Than Or Equal To. Each of them answers
   with 0 or 1 as well.
6. Increment and Decrement: `++` and `--`, written before a variable or
   after it. `++x` yields the value the variable holds afterwards and `x++`
   the value it held before, and both leave the variable changed by one.
   The value has to go back somewhere, so the operator takes a variable and
   not an expression: `++b[2]` has to wait for arrays.
7. Taking the Address of a variable with `&`, and reading or writing
   through a pointer with `*`. A pointer may be the target of an
   assignment: `*y= 14;` stores into the location `y` points at, and only
   into that location, so writing through a `char` pointer does not run
   over the value stored next to it. Only the `&` of a variable and the
   `*` of a pointer are accepted, and a pointer to a pointer has no type
   of its own yet.
8. Simple `print` statement.
9. If-Else Statements.
10. While Loops.
11. For Loops.
12. The condition of an `if`, a `while` or a `for` may be any expression
    and not only a comparison: an integer is false when it is zero and true
    otherwise, so `if (x)` tests one value and `while (*p)` walks a pointer
    to the end of what it points at.
13. Function Declarations and Calls. A function returns `void`, `char`, `int`
    or `long` and takes zero or one parameter. The parameter is visible in the
    function body, and an argument must match its type after widening. A
    function which returns a value must end with a `return` statement, and a
    void function cannot return one.
14. Global Variables. A declaration outside every function declares a
    variable which every function can use. One declaration may name
    several variables of the same type, written `int x, y, z;`, and the
    same is allowed for the variables declared inside a function. The
    `*` of a pointer type belongs to the declaration and not to the name,
    so it is written once: `char *b;` and not `char *b, *c;`. A name has
    to be declared before it is used, so a function can only reach the
    global variables declared above it. The variables are stored one
    after another in the order they are declared.
15. Pointer Arithmetic. An integer may be added to or subtracted from a
    pointer, and is scaled by the size of what the pointer points at, so
    `&c + 1` is the address of the next value of `c`'s type and not the
    address one byte along. Nothing else may be done with a pointer: it
    cannot be multiplied, divided or compared yet, and its address cannot
    be printed.
16. Function-local Variables. Variables declared inside a function are
    stored in that function's stack frame, are released when the function
    returns, and may have the same name as a variable in another function.
17. Function Parameters. A function may bind one typed parameter, such as
    `int add(int value)`, and callers may pass an expression of a compatible
    type. Parameterless functions may be called with empty parentheses.
18. Assignment Expressions. An assignment is an expression which yields the
    value stored, so `x= 5` may be used wherever a value may, as in
    `print x= 5;`. `=` binds less tightly than every other operator and to
    the right, so `c= a= 7` stores into `a` first and then into `c`. What
    may stand on its left is a variable or the location a pointer holds,
    and the value has to fit the target the same way it has to fit a
    variable.
