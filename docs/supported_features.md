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
   value two values along from where `y` points. A pointer may point at
   another pointer, so `char **row` holds the address of a `char *`: the
   `*` of it is that `char *` and a second `*` is the `char` at the end of
   it.
8. Simple `print` statement.
9. If-Else Statements.
10. While Loops.
11. For Loops.
12. The condition of an `if`, a `while` or a `for` may be any expression
    and not only a comparison: an integer is false when it is zero and true
    otherwise, so `if (x)` tests one value and `while (*p)` walks a pointer
    to the end of what it points at.
13. Function Declarations and Calls. A function returns `void`, `char`, `int`
    or `long` and takes zero to six parameters. Every parameter is visible in
    the function body as a local variable, and each argument must match the
    type of the parameter it is given to after widening. The arguments are
    worked out from the last to the first and passed in the registers the
    machine gives to arguments, so a call with more than six arguments is
    refused. A function which returns a value must end with a `return`
    statement, and a void function cannot return one.
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
17. Function Parameters. A function may bind up to six typed parameters, such
    as `int add(int a, int b)`, and a caller has to give one expression for
    each of them. Parameterless functions may be called with empty
    parentheses.
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
    does not fit in memory, and an array of a type which cannot hold a
    value such as `void a[2];` are refused. The elements may be pointers,
    so `char *words[3]` is a row of three of them.
21. Character Literals. `'a'` is one character between two single quotes,
    has the value of that character's code, and has the type `char`, so it
    may be assigned to a `char` and passed to a function like any other
    small number. The escapes `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`,
    `\\`, `\"` and `\'` stand for the characters they name. A literal
    which is empty, holds more than one character, is not closed by a
    quote before the end of the line or of the file, ends with a bare
    backslash, or uses an escape which is not in that list (so an octal
    code or a Unicode value) is refused.
22. String Literals. `"Hello world"` is zero or more characters between
    two double quotes, may hold spaces, and has the type `char *`. It
    stands for the address of its first character, its storage holds a
    zero byte after the last character, and it may be assigned to a
    pointer, indexed with `p[0]`, read through with `*p`, and walked with
    `p= p + 1` exactly like a pointer to a `char`. The same escapes as a
    character literal are known. A string which is not closed before the
    end of the line or of the file, or uses an unknown escape, is
    refused, and a string may not be given to `print` or stored into a
    number.
23. The `printchar()` Runtime Function. `printchar(x)` is called like any
    other function and prints the one character whose code is `x`,
    without a newline after it, so it is how the characters of a string
    are shown: `for (p= "hi"; *p != 0; p= p + 1) printchar(*p);`. Its
    argument must fit an `int` and it returns nothing; it is provided by
    lib/printint.c together with `printint()`.
24. Function Prototypes. A function may be declared without a body,
    before it is defined, so that it can be called from anywhere below the
    declaration: `int add(int a, int b);`. A later declaration of the same
    function has to agree with the prototype in the return type, the
    number of parameters and the type of each of them; a second body for a
    function which already has one is refused. The parameter names of a
    prototype are only placeholders, since it is the definition which
    gives them to the body.
25. Compiler Command Line. `nuocc [-vcST] [-o outfile] file [file ...]`
    compiles one or more source files. Unless `-c` or `-S` says to stop
    earlier, it also assembles what it generated and links the result into
    an executable, which is `a.out` or the file `-o` names; the runtime in
    `lib` is linked in with it, since that is where `printint()` and
    `printchar()` live. `-c` stops at one object file per input file, `-S`
    stops at one assembly file, `-v` prints each stage and the command it
    runs, and `-T` prints the tokens and the syntax tree of every input
    file. What a compilation produces is written where the compiler is
    run, named after the input file, and the object files made on the way
    to an executable are removed once it has been linked.
26. Pointers at Any Depth. A type records how many levels of pointer it
    has, so a `char **` is as ordinary a type as a `char *`: it may be
    declared, assigned, passed to a function, held in an array and read
    through more than once, and the type checks which already refuse a
    mismatched pointer apply at every level. Adding to a pointer of any
    depth moves it on by the size of what it points at, which for a
    pointer is eight bytes. Fifteen levels are as many as a type has room
    for.
27. Structs. `struct name { int x; char y; };` declares a type whose
    members are laid out one after another, and a variable of that type may
    be declared at the top level of the program or inside a function, with
    `struct name variable;` or with the definition in the declaration
    itself, `struct { int x; } variable;`. Every member is lined up to what
    its own type asks for, the room the whole type takes is rounded up to
    the strictest of them, and a global variable of the type is given that
    much storage. A member may be an array of a type which is not a struct,
    and the members may be named as they are declared, as
    `struct name { int x; };` does on its own.

    A member may be a pointer back at the struct it stands in, which is how
    a type which refers to itself is written:

        struct node { int value; struct node *next; };

    What a struct value cannot do yet is be read or written as a whole, or
    handed to a function: a member is not reachable until the next stage,
    so the members of a struct can be laid out, but nothing may look at
    them. An array of structs, arithmetic on a pointer to a struct, a
    struct which contains itself by value, a struct with no members, and a
    tag which is defined twice are all refused with a message saying so.
    Two pointers to structs of different types are not told apart yet.
