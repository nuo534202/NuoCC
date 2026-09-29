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
   The value has to go back somewhere, so the operator takes the name of a
   variable and not a location: `++b[2]` has to wait for an increment
   which can be given a place rather than a name.
7. Taking the Address of a variable with `&`, and reading or writing
   through a pointer with `*`. A pointer may be the target of an
   assignment: `*y= 14;` stores into the location `y` points at, and only
   into that location, so writing through a `char` pointer does not run
   over the value stored next to it. `&` takes the address of a variable
   or of an element of an array, written `&b[2]`, and undoes itself with
   a `*`, so `&*y` is `y`. `*` reads through anything which holds an
   address, so a parenthesised expression counts too: `*(y + 2)` is the
   value two values along from where `y` points. A pointer to a pointer
   has no type of its own yet.
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
19. Parenthesised Expressions. `(expression)` groups whatever stands inside
    it, so `(2 + 3) * (4 + 1)` is 25 and not 21. The parentheses may also
    hold a value which is then read through: `*(ptr + 2)` is the element
    two places along from where `ptr` points, the same as `ptr[2]`.
20. Arrays. `int a[5];` declares one name holding five `int`s one after
    another, at the top level of the program or inside a function, and the
    size is a fixed number of elements which may not be changed after the
    declaration. An element is read with `a[2]` and written with
    `a[2]= 7;`; the index is any expression, counts elements and not bytes,
    and is scaled by the size of one element. An array named on its own
    stands for the address of its first element, which is what `p= a;`
    stores and what lets an array be given wherever a pointer is wanted,
    and the address of an element is taken with `&a[2]`. A pointer may be
    indexed the same way, so `p[4]` reads just as `a[4]` does. Every
    element starts out zero. An array declares one name only, so
    `int a[2], b;` is refused, and the address of the whole array is not a
    value which may be stored into, so `a= p;` and `a++;` are refused. One
    dimension is known so far; a size which is zero, is not a number, or
    does not fit in memory, an array of a type which cannot hold a value
    such as `void a[2];`, and an array of a pointer type are all refused.
