# Supported Features

1. `char`, `int` and `long` variable Declaration and Assignment,
   including a pointer to any of them, written `char *b;`. A `char` holds
   0 to 255, an `int` four bytes and a `long` eight. A narrower value is
   widened where the types are mixed; a value which would have to be
   narrowed to fit its target is refused.
2. Arithmetic Operations: Addition, Subtraction, Multiplication, Division
   and the Remainder `%`, which is what is left of the first value once the
   second has been taken from it as many whole times as it holds, so
   `24 % 9` is 6. The remainder of a negative value follows the value it is
   taken from, so `-24 % 9` is -6. A `-` written before a value negates it;
   a `char` is widened to an `int` first, as an unsigned value has no
   negative one.
3. Bitwise Operations: And `&`, Or `|`, Xor `^`, Not `~`, Shift Left `<<`
   and Shift Right `>>`. A shift of a negative value fills the top bits in
   with zeroes.
4. Logical Operations: And `&&`, Or `||` and Not `!`. Each answers with 0
   or 1, and `!` is 1 when its value is zero. Each side is asked whether it
   holds rather than being combined with the other side, so the two sides
   need not be of one type: each has to be an integer or an address, and an
   address holds when it does not point at nothing, which is what lets
   `if (p && count > 0)` be written. The side which is not needed to know
   the answer is not worked out at all, so `0 && f()` never calls `f` and
   `1 || f()` never calls it either.
5. Comparison Operations: Equal To, Not Equal To, Less Than, Greater Than,
   Less Than Or Equal To, Greater Than Or Equal To. Each of them answers
   with 0 or 1, whatever the two values it is given are, so the answer is an
   `int` and may be used as one: `print (a == b);` and `x= (a < b) + 1;`
   both read the answer. Two pointers may be compared with each other as
   well as two numbers.
6. Increment and Decrement: `++` and `--`, written before a variable or
   after it. `++x` yields the value the variable holds afterwards and `x++`
   the value it held before, and both leave the variable changed by one.
   A pointer is changed by the room one value of what it points at takes
   rather than by one byte, so `p++` moves on to the next value of the type
   `p` points at, whatever size that value is. The value has to go back
   somewhere, so the operator takes the name of a variable and not a
   location: `++b[2]` has to wait for an increment which can be given a
   place rather than a name.
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
    statement, and it has to be given one; a void function leaves with a
    `return` which is given no value, as `return;` in the middle of a body
    does, and it cannot return one. A call may stand inside an expression,
    with the values worked out before it kept for afterwards, since a
    function is not allowed to disturb the registers of the function which
    called it.
14. Global Variables. A declaration outside every function declares a
    variable which every function can use. One declaration may name
    several variables of the same type, written `int x, y, z;`, and the
    same is allowed for the variables declared inside a function. Every
    name of the list carries its own type: the `*`s and the array size
    written with a name are what give it a pointer or an array on top of
    the base type, so `char *b, c;` declares a pointer and a plain `char`.
    A name has to be declared before it is used, so a function can only
    reach the global variables declared above it. The variables are stored
    one after another in the order they are declared.
15. Pointer Arithmetic. An integer may be added to or subtracted from a
    pointer, and is scaled by the room one value of what the pointer points
    at takes, so `&c + 1` is the address of the next value of `c`'s type and
    not the address one byte along. What a struct or a union takes is the
    room its definition gives it, so a pointer to one is moved on by that
    whole room rather than by the room of one member: at the first element
    of an array of structs, `p + 1` reaches the second one. An integer added
    to a pointer through `+=` or `-=` is scaled the same way, so `p += 2`
    moves on by two values of the type `p` points at and not by two bytes.
    Two pointers may be compared with each other whatever they point at,
    since the question is which address is the lower one, and that is how a
    pointer is asked whether it points at anything: `if (p != NULL) ...`.
    An increment or a decrement of a pointer moves on by one value of what
    it points at as well, so `p++`, `++p`, `p += 1` and `p= p + 1` all move
    an `int *` on by four bytes and a `struct point *` on by the room a
    struct point takes. Nothing else may be done with a pointer: it cannot
    be multiplied or divided yet, and the address it holds cannot be
    printed.
16. Function-local Variables. Variables declared inside a function are
    stored in that function's stack frame, are released when the function
    returns, and may have the same name as a variable in another function.
    Nothing is put in that storage before the function uses it, so a local
    holds whatever was there until a value is stored in it.
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
    indexed the same way, so `p[4]` reads just as `a[4]` does. The
    elements of a global array start out zero unless it is given values
    where it is declared, while those of a local one hold whatever the
    storage it was given held until a value is stored in each of them. An
    array may stand in a declaration beside
    other names, since each name brings its own size: `int a[2], b;`
    declares an array and a plain variable. What an array's name stands for
    is a value and not a place, so it cannot be stored into, changed, or
    asked for its own address: `a= p;`, `a++;`, `++a;` and `&a` are all
    refused with a message saying which of them was written, while an
    element is a place of its own, so `&a[2]`, `a[2]= 7;` and `a[2] += 7;`
    are all taken. A parameter which is given an array is written as a
    pointer, since what the function is given is the address of the first
    element rather than the array, so `int sum(int *values, int count)` is
    the way to write one. One dimension is known so far; a size which is
    zero, is not a value the compiler can work out while it is compiling the
    program, or does not fit in memory, and an array of a type which cannot
    hold a value such as `void a[2];` are refused. The elements may be
    pointers, so `char *words[3]` is a row of three of them.
21. Character Literals. `'a'` is one character between two single quotes,
    has the value of that character's code, and has the type `char`, so it
    may be assigned to a `char` and passed to a function like any other
    small number. The escapes `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`,
    `\\`, `\"` and `\'` stand for the characters they name, and a
    character may also be named by its code: in octal, as up to three
    digits, so `'\101'` is `'A'`, or in hexadecimal after an `x`, as in
    `'\x41'`. A literal which is empty, holds more than one character, is
    not closed by a quote before the end of the line or of the file, ends
    with a bare backslash, or uses an escape which is not one of those, or
    names a code larger than a byte, is refused.
22. String Literals. `"Hello world"` is zero or more characters between
    two double quotes, may hold spaces, and has the type `char *`. It
    stands for the address of its first character, its storage holds a
    zero byte after the last character, and it may be assigned to a
    pointer, indexed with `p[0]`, read through with `*p`, and walked with
    `p= p + 1` exactly like a pointer to a `char`. The same escapes as a
    character literal are known, so a string may hold a zero byte or any
    other code of its own. Strings written one after another are one string,
    which is how a string too long for one line is written down over
    several: `"hello " "there"` is `"hello there"`. A string which is not
    closed before the end of the line or of the file, or uses an unknown
    escape, is refused, and a string may not be given to `print` or stored
    into a number.
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
25. Compiler Command Line. `nuocc [-vcSTM] [-o outfile] file [file ...]`
    compiles one or more source files. Unless `-c` or `-S` says to stop
    earlier, it also assembles what it generated and links the result into
    an executable, which is `a.out` or the file `-o` names; the runtime in
    `lib` is linked in with it, since that is where `printint()` and
    `printchar()` live. `-c` stops at one object file per input file, `-S`
    stops at one assembly file, `-v` prints each stage and the command it
    runs, `-T` prints the tokens and the syntax tree of every input file,
    and `-M` prints the names that file declared: the globals with the
    values they start with, the functions with the types they take, the
    typedefs and the enum values, each with the size it takes in bytes.
    What a compilation produces is written where the compiler is run, named
    after the input file, and the object files made on the way to an
    executable are removed once it has been linked.
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
    much storage. A member may be an array of any type which takes room, a
    struct or a union included, and the members may be named as they are
    declared, as `struct name { int x; };` does on its own.

    A member may be a pointer back at the struct it stands in, which is how
    a type which refers to itself is written:

        struct node { int value; struct node *next; };

    A struct value cannot be read, passed to a function or returned as a
    whole yet, and one may be written as a whole only from a place holding
    one of the same type, as item 69 describes. Members can be read and
    written as described below, and an array of structs may be declared and
    every element of it reached, as item 68 describes. A struct which
    contains itself by value, a struct with no members, and a tag which is
    defined twice are all refused with a message saying so. Pointers to
    different struct types are distinct even when their members have the
    same names and layout.

28. Struct Member Access (acwj Stage 32, `32_Struct_Access_pt1`).
    `item.number` selects a member of a struct value and `ptr->number`
    selects a member through a pointer to a struct. Both work for global
    and local variables, and a member can be read, assigned, used in an
    expression or passed to a function. Reading an `int` member preserves
    its sign, reading a `char` yields its unsigned value, and a store writes
    only the bytes of that member. A function may take or return a struct
    pointer, and negative `int` return values retain their sign at a call.

    Member selection may be chained: `box.data.number` accesses a nested
    struct and `head.next->next->value` follows linked nodes. Parentheses
    and dereferencing work with selection, as in `(*ptr).number` and
    `(*indirect)->number`. An array member decays to the address of its
    first element and can be indexed, as in `box.slots[2]`; a pointer member
    can be indexed too. `&item.number` and `&ptr->number` take the address
    of a scalar member, while `&box.slots[2]` takes an element's address.
    A nested struct's address can be taken and used as a struct pointer.

    `.` requires a struct value and `->` exactly one level of pointer to
    a struct. Unknown or missing member names, duplicate member names,
    assigning to an array member as a whole, and incompatible struct
    pointers in assignments, calls, returns and prototypes are refused.
    Incrementing a member and taking the address of an entire array member
    remain unsupported, while a whole struct may be given what another place
    holding one holds, as item 69 describes. A pointer to a struct is stepped
    on like any other pointer, by the room the struct takes, and an element
    of an array of structs is reached as `path[1].x` or `(&path[1])->x`.

29. Unions (acwj Stage 33, `33_Union`). `union name { ... };` declares a
    composite type whose members all start at offset zero, and an anonymous
    union may be defined where a variable is declared. A union member can
    be read and written in exactly the ways already supported for a struct
    member: through `.`, through `->`, when nested in a struct or another
    union, and when it is an array or a pointer. Writing one member changes
    the bytes another member reads when they overlap; the test relies on
    this to show a `char` array, an `int` and a `long` sharing one value.

    The size of a union is the size of its largest member, rounded up to
    the union's alignment, and its alignment is the strictest alignment
    any member asks for. `union same` and `struct same` cannot share a
    tag, and pointers to different composite types remain distinct. A
    whole composite value still cannot be read as a value, passed by value
    or returned, while one place may be given the contents of another as
    item 69 describes and an array of them may be declared and reached as
    item 68 describes.

30. Enums and Typedefs (acwj Stage 34, `34_Enums_and_Typedefs`). An
    `enum name { ... };` gives names to numbers, and each name may be
    written wherever a number may, so in `enum fred { apple= 1, banana };`
    the name `banana` stands for 2. Every name counts one past the one
    before it, starting at zero, and a number which is written out sets
    where the next name counts from, as `pear= 10` makes the name after it
    stand for 11. A name between the braces is a name for a value and not
    a place which holds one, so it cannot be stored into, and it cannot be
    used again for a variable or for another value.

    The tag shares one namespace with structs and unions, so a tag already
    used by one kind cannot be reused as another and a tag cannot be
    defined twice. A variable of an enum is an int, so `enum fred var1;`
    declares one and `enum { red, green, blue } colour;` defines a type
    without a tag and a variable of it in the same declaration. An enum
    written without braces names a tag which is already known, and a tag
    which is not known is refused.

    `typedef type name;` makes `name` another way of writing `type`, so
    `typedef int FOO; FOO x;` declares an int named `x`. The type may be
    any the language has, including a pointer, a struct, a union, an enum
    or another typedef, so `typedef struct bar BAR;` makes `BAR` stand for
    `struct bar` and `typedef FOO COUNT;` makes `COUNT` stand for `int` as
    well. A typedef name may be written wherever a type may, at the top
    level of the program or inside a function, and it is the type behind
    the name which the type checks see. A name which is already used for
    something else cannot be made a typedef.

31. The C Pre-Processor and `extern` (acwj Stage 35, `35_Preprocessor`).
    Every source file is put through the system C pre-processor before it
    is scanned, so comments, `#include`, `#define`, `#ifdef` and the rest
    of the pre-processor's language work. A `//` or `/* ... */` comment is
    taken away before the compiler reads the program, so a comment may
    stand anywhere a space may. The pre-processor is told not to look in
    the system's include directory, because the header files there use
    parts of C this compiler does not have; the directory it does search
    is `lib/include`, which holds `stddef.h` and `stdio.h` to begin with.
    A header file may include another one and may guard itself against
    being read more than once, which is how a header used twice in one
    program does no harm. The lines the pre-processor writes to say which
    file and line the text after them came from begin with `#` in the first
    column of the line, and the scanner steps over them. A `#` anywhere else
    is a character the language has no use for and is refused, so that a
    stray one is reported where it stands rather than swallowing the rest of
    its line.

    `extern` declares a name whose storage is given in another source
    file, so the name may be used here but no storage is emitted for it.
    It has to come before the type, as in `extern int shared;`, at the top
    level of the program or inside a function. A name declared at the top
    level may be declared again as the one thing it is not yet, so
    `extern int shared;` may stand before or after the declaration which
    gives the name its storage; the two declarations have to agree about
    the type of the name and leave one name behind, while any other name
    declared twice at the top level is refused. A name declared `extern` is
    addressed the way a global is, so it is the definition, wherever it
    stands, which gives it room.

32. Break and Continue (acwj Stage 36, `36_Break_Continue`). `break;`
    leaves the loop it stands in at once, and `continue;` begins that
    loop's next pass. A break jumps to the code after the loop, so a loop
    within a loop leaves only the one the break is written in. The next
    pass of a `while` loop begins at its condition, and the next pass of a
    `for` loop begins at the step written at the end of it, so a continue
    in a for loop still runs that step. Written where no loop is around
    it, either one is refused with `there is no loop to break out of!` or
    `there is no loop to continue!`.

33. Switch Statements (acwj Stage 37, `37_Switch`). `switch (expression)`
    chooses one arm out of a list of `case` arms: the value of the
    expression is worked out once and compared against the value each
    `case` names, and the arm whose value matches is run. The expression
    has to be an integer, and every case value has to be an integer
    constant, which is a literal or the name an enum gave to one. An arm
    which does not end in a `break` falls through into the arm after it,
    and a `break` leaves the switch altogether, so a `break` is allowed in
    a switch the way it is in a loop. A `continue` written in a switch
    still belongs to the loop the switch stands in, so it begins that
    loop's next pass rather than leaving the switch.

    A `default` arm runs when no case value matches and has to be the last
    arm. A switch without a default arm runs no arm and goes on with the
    statement after it when nothing matches. A switch with no arms, one on
    an expression which is not an integer, a case value which is not an
    integer constant, a case value written twice, and an arm written after
    the default arm are all refused with a message saying so.

34. Single Statements and Expression Lists (acwj Stage 38,
    `38_Dangling_Else`). The body of an `if`, an `else`, a `while` or a
    `for` is one statement, so braces are only needed where more than one
    statement has to stand there:

        if (x > 5)
            x = x - 16;
        else
            x++;

    A block written between braces is a statement like any other, so it may
    stand wherever a statement may, including nested inside another block.
    The statements of a `case` or a `default` arm need no braces either:
    they are written one after another until the next arm or the end of the
    switch. An `else` is taken by the `if` it stands nearest, so

        if (a) if (b) x = 1; else x = 2;

    leaves the `else` with the inner `if`, and a chain of decisions is
    written by putting an `if` after the `else`:

        if (a) x = 1;
        else if (b) y = 2;
        else z = 3;

    The first and the last clause of a `for` loop are lists of expressions
    which run in the order they are written, so
    `for (x = 0, y = 1; x < 6; x = x + 1, y = y + 2)` initializes and steps
    two variables at once. Either of those clauses may be left empty, as
    `for (; x < 5; x = x + 1)` does.

35. Declaration Lists and a Type for Every Name (acwj Stage 39,
    `39_Var_Initialisation_pt1`). The type at the front of a declaration
    is only the base of every type the declaration gives: each name adds
    its own `*`s and its own `[size]`, so one declaration may name plain
    values, pointers and arrays at once.

        int   a, *b, **c;
        char *e, f;             /* e is a pointer, f is a plain char */
        int   g, h[3], *i[4];

    The same holds wherever a type names something. A member of a struct
    or a union may be a pointer or an array of its own, and a function may
    take a pointer of its own with every parameter: `int f(int a, char
    *b);`. A typedef may name a pointer, so `typedef char *charptr;` makes
    `charptr` stand for `char *`, and a function may return a pointer, so
    `char *g();` declares one. Initializers are not part of the language
    yet, so a value may not be given where a name is declared.

36. Global Variable Initialisation (acwj Stage 40,
    `40_Var_Initialisation_pt2`). A global variable may be given the value
    or the values it starts with where it is declared. What stands there is
    an expression which the compiler works out while it is compiling the
    program, so `int x= 10 + 6;` is given the value 16 and `char letter=
    'a' + 2;` the value 99. A `char`, an `int` or a `long` is given a number
    which has to fit it, so `int x= 2;` and `long big= 100000;` are taken
    while `char c= 300;` is refused. A variable of type `char *` is given
    a string: its characters are put in
    the data section and the address of them is the value the variable
    starts with, so `char *str= "Hello";` begins at a `'H'` and `char
    *empty= "";` at a zero byte. A struct or a union cannot be given a
    value where it is declared, and neither can an `extern` name, whose
    storage belongs to another source file; a local name is given its value
    by an assignment instead, which the next item describes.

    An array is given a list of values between braces. The values left over
    when there are fewer of them than the array has room for start at zero,
    so `int e[4]= { 1, 4, 17 };` holds `1`, `4`, `17` and `0`; the size may
    be left out, in which case the array holds exactly the values it was
    given, so `int f[]= { 9, 8, 7, 6 };` holds four of them. A list of
    strings fills a row of pointers, so `char *d[]= { "apple", "banana",
    "peach" };` is three of them. An array with a size but no values starts
    out all zero, as it always has. A list with more values than the array
    has room for, a value the compiler cannot work out while it is compiling
    the program, an `extern` name given a value, a local array given a list
    of values, and an array written with neither a size nor values are all
    refused with a message saying so.

37. Local Variable Initialisation (acwj Stage 41, `41_Local_Var_Init`). A
    local variable may be given the value it starts with, and that value is
    any expression and not only a literal: `int x= 3, y= 14;`, `int z= 2 * x
    + y;` and `char *str= "Hello world";` all work. The declaration becomes
    an assignment which runs where it stands, so the value is worked out
    again every time the statement is reached, and a name may be written in
    terms of the ones declared before it in the same declaration, as `y= x +
    3` is above. The value has to fit the type the variable was declared
    with, and the same rules as an assignment written by hand then apply.

    A local variable may also be given no value, in which case it holds
    whatever its storage held until a value is stored in it. An array is
    given its values only when it is global, so a local array may not be
    written with a list of values yet.

38. Type Casting, `void *` and NULL (acwj Stage 42, `42_Casting`). A cast
    names a type between parentheses in front of one expression, and the
    value is taken to be of that type from then on: `(char)wide` keeps the
    low byte of a wider value, `(int)c` makes a `char` a value which may be
    printed, and `(char *)&a` makes the bytes of an `int` readable one at a
    time. A cast may name any type which holds a value, so a struct, a union
    and a plain `void` are refused. It holds on to the one expression which
    follows it, as a unary operator does, so `(char)x + 1` adds one to the
    value which was cast.

    `void *` is the one pointer which stands for any other: it may be stored
    where any pointer is wanted and any pointer may be stored where one is
    wanted, so `int *p= &a; void *v= p; int *q= (int *)v;` works in both
    directions. A value is kept in a register as the whole of it and read at
    the width its type says, so a cast to a narrower type keeps the low bits
    of the value it was given: `(char)wide` keeps the low byte and
    `(int)wide` the low four bytes. What is kept is read back as a number of
    the new type, so a cast to an `int` of a negative number is that negative
    number still, while `(char)-1` is 255, since a `char` has no negative
    ones. A store keeps the bits which fit the storage it is made into.

    A pointer is given a number only behind a cast, which is how a pointer
    which points at nothing is written: `char *p= (void *)0;`. The header
    files define `NULL` as `(void *)0`, so `int *p= NULL;` and `char *text=
    NULL;` say the same thing. A cast to a struct, a union or void, a cast
    whose type does not agree with the variable it is given to, and a number
    given to a pointer without a cast are all refused with a message saying
    so.

39. Compound Assignments (acwj Stage 43, `43_More_Operators`). `+=`, `-=`,
    `*=`, and `/=` combine the value the target holds with the value on
    their right and store the result back in the target, so `y += x;` adds
    `x` to `y`. Each of them is an expression whose value is the one it
    stored, so `print x += 4;` stores and then prints 5, and each takes
    what stands on its right first, so `x += y += 3;` adds three to `y`
    before adding the new `y` to `x`. What may stand on the left is what an
    assignment may store into, so a variable, the location a pointer holds
    and a member all work: `*p += 5;`, `p[0] *= 2;` and `box[x++] += 1;`.

    The address of the target is worked out once, so an index or a call
    written inside it runs once, as it does for a plain assignment. The
    value on the right has to fit the target under the operator it combines
    with, so `c += x;` is refused when `c` is a `char` and `x` is an `int`,
    the same way `c= c + x;` is, since the result would have to be narrowed
    to fit.

40. Negative Constants (acwj Stage 43, `43_More_Operators`). Wherever the
    compiler has to know a number while it is compiling the program, that
    number may be written with a minus in front of it: an enum value, the
    value a global variable or one element of a global array starts with,
    and the value of a `case`. So `enum numbers { low= -1, zero };` counts
    from `-1`, `int x= -5;` and `int table[3]= { -1, 0, 1 };` hold negative
    numbers, and `case -1:` may be written. A `char` holds 0 to 255, so a
    negative value is refused there unless a cast says to keep the low byte
    of it, as `char byte= (char)-1;` does, which stores 255.

41. Constant Folding (acwj Stage 44, `44_Fold_Optimisation`). An operation
    whose operands the compiler already knows is worked out while the
    program is being compiled, so `int x= 2000 + 3 + 4 * 5 + 6;` stores
    2029, `print 2 * 3 + 4;` prints 10, and no code is generated to work
    any of it out. The folding reaches every operation on two numbers:
    `+`, `-`, `*`, `/`, `&`, `|`, `^`, `<<`, `>>`, the comparisons, `&&`
    and `||`, along with `-`, `~` and `!` in front of a number. It works
    from the inside out, so `2 * 3 + 4` becomes `6 + 4` and then `10`, and
    it reaches into the condition of a statement, the value of a switch,
    the arms of a switch and the arguments of a call.

    A folded operation always holds the value it would have had when the
    program ran. The machine works in whole registers, which are wider than
    an int, so an operation is worked out in a wider number and is only
    folded when the answer fits an int: `2000000000 + 2000000000` is
    therefore left for the program, and gives 4000000000 when it runs,
    while `1000 * 1000 * 1000` is folded. A division by zero and a shift of
    a whole register or more are left for the program for the same reason:
    they have to fail, or wrap round, the way they do at run time. What a
    global variable or a `case` is given still has to be a number written
    where it stands, since the folding happens once the program has been
    read rather than while it is being read.

42. Constant Expressions in Declarations (acwj Stage 45,
    `45_Globals_Again`). Every value which the compiler has to know while it
    is compiling the program is written as an expression and worked out
    there and then, since the folding of the last item is what reads it. So
    a global variable may be given an expression of numbers, an array may be
    given a size which is an expression of them, and each of an array's
    values, an enum's value and a `case` may be one as well:

        enum sizes { SMALL = 2 * 3, LARGE = 100 + 24 };
        int text_length= 512 + 1;
        int fred[2 + 3];
        int table[]= { 1 + 1, 2 * 2, SMALL };

        switch (total) {
            case 10 + 6: { ... }
        }

    The value still has to be one the compiler can work out, so
    `int doubled= count * 2;` is refused when `count` is a variable,
    however simple it looks, and so is a size which is a variable. A cast
    may stand in front of the value, which is what lets a value the variable
    could not hold on its own be given to it, as `char wrapped= (char)('a' +
    1024);` keeps the low byte of the value and `char *none= NULL;` writes a
    pointer which points at nothing. A string is a value of its own, and one
    given where a number is wanted is refused.

43. Void Parameters, and Numbers of Any Radix (acwj Stage 46,
    `46_Void_Functions`). A parameter list may be the one word `void`,
    which says the same thing as an empty list: the function takes
    nothing, so `int fred(void);` may be written as well as `int fred();`.
    A `void *` parameter is a parameter like any other, so `int count(void
    *start, int limit);` declares two of them, and a `void` with a name
    after it is still refused, since a parameter has to hold a value.

    A number may be written in another radix than ten. One which begins
    with a zero is written in octal, so `0173` is 123, and one which
    begins with `0x` is written in hexadecimal, so `0x7b` is also 123.
    A digit which the radix has no room for, a `0x` with no digits after
    it, and a number too large to be held in an `int` are all refused,
    each with a message saying what is wrong with it.

44. The Size of a Value (acwj Stage 47, `47_Sizeof`). `sizeof` answers with
    the room a value takes, in bytes, and is written with the type in
    parentheses: `sizeof(char)` is 1, `sizeof(int)` and `sizeof(char *)` are
    4 and 8, and a struct is as large as its members make it, so a
    `struct pair { int first; char second; };` is 8 bytes once its five
    bytes are lined up to what they hold. The answer is a number the
    compiler knows, so it may stand wherever a constant may: it may be the
    size of an array, the value a global variable starts with, or part of
    any other constant expression, as `char buffer[sizeof(int) * 4];` shows.

    What stands between the parentheses may also be a value rather than a
    type, in which case it says which type is asked about and is not worked
    out at all: `sizeof(count= 5)` is 4 and leaves `count` as it was. An
    array named on its own asks about the room the whole array takes rather
    than about the address it stands for, so `sizeof(buffer)` above is 16
    where `sizeof(&buffer[0])` would be the size of a pointer. A type which
    cannot hold a value, such as `void`, is refused, and so is a `sizeof`
    which is not followed by a parenthesis.

45. Static Names (acwj Stage 48, `48_Static`). `static` may stand in front
    of the type of a variable or a function declared at the top level of a
    program, and says that the name belongs to the source file it is
    written in: the assembler is not told another file may reach it, so two
    files may each declare a static name of the same one and each sees only
    its own. Everything else about the name is as it is for any global one,
    so a static variable may be given the value it starts with and a static
    function may be called from anywhere in its file:

        static int counter= 0;
        static int fred(void) { return(counter++); }

    A name declared static inside a function is refused, as are the two
    words `extern` and `static` written together, which would say both that
    the storage belongs to another file and that the name belongs to this
    one. The third kind of static name a C compiler has, a variable inside
    a function whose value lives on between calls to it, is not part of the
    language: a name declared static has to be declared where the whole
    file can be seen.

46. The Ternary Operator (acwj Stage 49, `49_Ternary`). `condition ? a : b`
    works out the condition and then only the value it chooses, so `x= 1 ?
    5 : counted();` stores 5 and never calls `counted()`. It is an
    expression like any other, so it may stand inside a larger one, as
    `print 23 * (1 ? 3 : 4) - 18;` does, and both of its values are worked
    out from the place it stands, so a call may be written on either side
    without parentheses.

    It binds more loosely than every operator but the assignments, and to
    the right: `a ? b : c ? d : e` reads the second `?` as part of the value
    taken when `a` does not hold, so the two conditions choose between three
    values. The two values have to be of one type, and the wider of the two
    is the type of the whole expression, so `0 ? 'a' : 4000` is 4000; an
    expression whose two values are calls which answer with nothing is one
    of those itself, so `flag ? printint(1) : printint(2);` may be written
    as a statement. A number where a pointer is wanted is refused, since the
    two values cannot be made to agree.

47. Consecutive Cases, and Arrays as Arguments (acwj Stage 50,
    `50_Mop_up_pt1`). A `case` may be written with nothing after it, in
    which case it falls into the case written below it, so several values
    may share one body:

        case 1:
        case 2:
            result= 10;
            break;

    The name of an array stands for the address of its first element, so it
    may be given wherever a pointer is wanted, and that includes a function
    call: `first(word)` passes the address of the first character of `word`,
    and a function may walk it with an index, as `values[i]` does.

48. Arrays and Pointers Are One Value (acwj Stage 51, `51_Arrays_pt2`). An
    array's name stands for the address of its first element, so an array
    and a pointer to it may be used the same way: `p= a;` stores the
    address, `p[2]` and `a[2]` read the same element, `p[2]= 30;` writes it
    through either name, `*(a + 1)` and `(a + 1)[0]` read the element after
    the first, and an array may be given to a function, which takes it as a
    pointer and may walk it with an index. A pointer may itself be pointed
    at, so `row= &ptr;` makes `row[0][1]` the value one along from where
    `ptr` points. What an array's name stands for is a value and not a
    place, so what cannot be done with a place is refused: it cannot be
    stored into (`a= p;`), changed (`a++;`, `++a;`) or asked for its own
    address (`&a`), while an element is a place like any other, so its
    address may be taken and it may be the target of an assignment or of a
    compound one. An array's size is all of its elements and a pointer's is
    its own, which `sizeof` answers with, and a parameter which is given an
    array is written as a pointer.

49. Chains of Selections (acwj Stage 52, `52_Pointers_pt2`). A name, an
    element of an array or a member may be followed by any number of
    selections, one after another, which is how a chain of linked values is
    followed:

        ptr->next->next->val
        list[0]->val
        (*ptr).next->val
        box.link->val

    `.` selects a member of a struct or a union value and `->` selects one
    through a pointer to one, so the two may be written one after the other
    as the values they reach allow. `list[0]->val` reads the value through
    the pointer held in the first element of an array, and `box.link->val`
    the one held in a member of a union. What an increment or a decrement is
    written on has to be the name of a variable, so a value read through a
    pointer or taken from a member is refused, and so is an increment of an
    increment: `value++ ++` has no name for its new value to go back into.

50. Strings Together, Empty Statements, and the Rest (acwj Stage 53,
    `53_Mop_up_pt2`). Strings written one after another are joined into one,
    so a string too long for its line may be written over several.
    A `;` on its own is a statement which does nothing, which is how a loop
    or a branch with nothing to do is written: `while (0) ;`. A name
    declared at the top level may be declared again as the one thing it is
    not yet, which is what a global name and an `extern` name of the same
    one are to each other, and only that pair may stand together. The
    operands of `&&` and `||` are asked whether they hold rather than being
    combined, so they need not be of one type as long as each is an integer
    or an address. A function which answers with nothing may leave with a
    `return` which is given no value, and a function which answers with a
    value has to be given one to return.

51. Deep Expressions (acwj Stage 54, `54_Reg_Spills`). An expression may
    stand as deep as it likes: when more values have to be held at once
    than there are registers to hold them, the ones which are not needed
    for the moment are put aside in the frame of the function and brought
    back when they are needed again. So a chain such as `a + (b + (c + (d +
    (e + (f + (g + h))))))` is worked out as it is written, and a call may
    stand in the middle of one, with the values worked out before it kept
    for afterwards:

        x= 3 * add(2, 3) - 5 * add(4, 6);

    What is put aside is done with at the end of the statement it was put
    aside in, so a loop may work out a deep expression on every pass. The
    room for this is part of each function's frame, so it costs a little
    space in every function whether it is used or not, and an expression
    which needs more values standing at once than that room holds is refused
    with a message saying so rather than being worked out wrongly.

52. Lazy Evaluation (acwj Stage 55, `55_Lazy_Evaluation`). The right
    side of `&&` and `||` is worked out only when the left side did not
    already settle the answer: a left side which is zero settles `&&` as
    zero, and one which is not zero settles `||` as one. So
    `0 && f()` never calls `f`, `1 || f()` never calls it, and the side
    which is skipped may be the one which would go wrong:

        if (pointer && *pointer == 1) { ... }

    reads through the pointer only once it is known not to point at
    nothing. The two operations still answer with 0 or 1, and a chain such
    as `a && b && c` is read from the left, each side being worked out only
    when the ones before it did not settle the answer.

53. Local Arrays (acwj Stage 56, `56_Local_Arrays`). An array may be
    declared inside a function, where it lives in that function's frame
    beside the other local variables: `int ary[5];` takes twenty bytes of
    the frame, and writing an element of it leaves the variables declared
    around it alone, however the compiler lays them out. Its name stands
    for the address of its first element like any other array's, so it may
    be given to a function, indexed, and asked for the room it takes with
    `sizeof`. What a local array cannot be given is the values it starts
    with, so `int ary[5]= { 1, 2, 3, 4, 5 };` is refused and the values have
    to be stored into its elements; a size is therefore the one thing a
    local array must have, and `int ary[];` is refused for the same reason a
    global one would be. An array of composite values may be declared here
    as it may at the top level, with the one difference that its elements
    cannot be given values and are cleared to nothing where the declaration
    stands, which is what item 68 describes.

54. Addresses Compared, and Files of Storage (acwj Stage 57,
    `57_Mop_up_pt3`). Two pointers may be compared with each other, and a
    pointer may be compared with `NULL`, which is how a pointer is asked
    whether it points at anything:

        if (a == NULL || b == NULL || c == NULL) { ... }

    Each `==` binds more tightly than the `||`, which it also does when a
    cast stands on one side of it, since a cast is written inside
    parentheses of its own: `if ((char)x == y || z == w)` reads the way it
    looks. The answer of a comparison is an `int` whatever the two values
    are, so it may be printed or added to; and an integer added to a pointer
    through `+=` or `-=` is scaled by the size of what it points at, exactly
    as it is with `+` and `-`.

    A source file of a program need not declare a function of its own: it
    may hold the storage which the other files reach into, declaring the
    names it needs as `extern`:

        /* in one file */          /* in another file */
        extern int shared;         int shared= 7;
        extern int values[3];      int values[3]= { 1, 2, 3 };

    The files are compiled together, so a name declared `extern` in one of
    them is found in whichever of the others defines it.

55. Pointer Increments, and the Remainder (acwj Stage 58,
    `58_Ptr_Increments`). An increment or a decrement of a pointer moves on
    to the next value of the type it points at, whether it is written before
    or after the name and whether the value it yields is the new one or the
    old: `p++`, `++p`, `p--` and `--p` all step by the room one value takes,
    so `*(p++)` reads the value the pointer stood at and then moves it on,
    and a `char *` steps by one byte where an `int *` steps by four.

    The remainder `%` answers with what is left of its first value once the
    second has been taken from it as many whole times as it holds: `24 % 9`
    is 6 and `31 % 11` is 9. `%=` stores that answer where the first value
    stands, as the other compound assignments do. The remainder binds as
    tightly as `*` and `/`, so `2 + 24 % 9 * 2` is 14, and it is only taken
    of numbers: the remainder of one address by another is refused, as is a
    remainder of nothing, which the machine is left to stop the program for.

56. The Shapes Which Are Easy to Get Wrong (acwj Stage 59, `59_WDIW_pt1`).
    A dereference reaches the element before the value it reads, so
    `*rows[1]` reads the character `rows[1]` points at and not one of
    `rows[0]`: `*rows[1]` and `(*rows)[1]` are two different values. A name
    whose storage is in another file is stored into, read and asked for its
    address the same way any other global name is, so `shared = 5;`,
    `&shared` and `values[2] = 7;` all work on names declared `extern`. A
    value narrower than a register keeps its own value through every
    operation it takes part in: a negative number compares as a negative
    one, and what a cast leaves behind is read back as a number of the new
    type rather than as its bits alone.

57. Values Held Inside an Expression (acwj Stage 60, `60_TripleTest`). A
    value which has been worked out as part of an expression keeps its
    place until it is used, however many operations stand around it: only
    the end of a statement throws away what the statement was holding. A
    target and the expression on the right of a compound assignment may
    therefore both be held at once, and that expression may itself hold a
    comparison, a ternary expression, a call or a logical operation
    without disturbing the values the rest of the statement is keeping:

        total += (fred > 0 && jim > 0) ? add(fred, jim) : 0;
        k += (jim > 4) ? fred : jim;

    Where a value goes when there is no register left to hold it, and when
    it comes back, is what item 51 describes; this item is about the values
    which are in registers still, which nothing else in the same statement
    may take away from the expression which is holding them.

58. Nested Members, Members of Composites, and Member Array Sizes (acwj
    Stage 62, `62_Cleanup`). The thing in front of a `.` or a `->` may
    itself be a member of a composite or a pointer to one, so the offset of
    the inner member is added to the offset which has already been worked
    out: `thing.member1.age_in_years`, `pointing->member1.age_in_years` and
    `(&local)->member1.age_in_years` all name a member of the struct which
    the outer struct holds, wherever the outer one is. The room a struct
    takes is worked out from the members it holds, a member which is a
    struct or a union included, each standing at the alignment of its own
    type: a union which is a member takes as much room as its widest member
    and shares that room with the rest of them. The size of an array which
    is a member is an expression the compiler works out while it is
    compiling the program, so `int table[2 * 3];` is a member which holds
    six elements.

59. A Back End Which Is Not a Machine (acwj Stage 63, `63_QBE`). The
    compiler may also be built to write the intermediate language of
    [QBE](https://c9x.me/compile/) rather than the assembly of a machine:
    `cmake -DNUOCC_TARGET=qbe ..` picks the QBE target, and every program is
    then written into a file ending in `.q`, which QBE compiles into the
    assembly of whichever machine the program is built for. Nothing of the
    language changes with it, so every program the machine targets build is
    built the same way by this one and prints the same thing. Every value of
    the intermediate language has a name of its own, so where the machine
    targets work out which register a value is given, this one names the
    value and leaves the rest to QBE, which builds the SSA form, allocates
    the registers, and leaves assembly that is smaller than the assembly the
    machine targets write. QBE has to be on the path, and the assembler and
    linker the compiler already uses do the rest.

60. A Second Machine, Which Is Not This One (acwj Stage 64,
    `64_6809_Target`). The compiler may also be built to write the assembly
    of the Motorola 6809, an eight bit machine of 1980:
    `cmake -DNUOCC_TARGET=m6809 ..` picks that target, and every program of
    the language is built for it as well. Nothing of the language changes:
    the same programs are compiled, and they print what the other targets
    print wherever the machine gives a type the same width and holds the
    bytes of a value in the same order. The machine has one accumulator and
    instructions which always work with it, so where the other targets hand
    out registers, the four registers the walk of the tree asks for are
    four places in the frame of the function, and a value is worked out in
    the accumulator and left in the place its register keeps it. An `int`
    is two bytes and a pointer two as well, since the machine reaches no
    more than 64K of memory, and a `long` is four; the arithmetic the
    machine has no instruction for, which is a multiplication, a division,
    a remainder and the negative of a `long`, is worked out by the runtime
    in [lib/6809](lib/6809) that every program of this target is built
    with. A program of this target is put together by joining the runtime
    and the code the compiler wrote and assembling the whole of it, so it
    is built by one command rather than assembled and linked, `-c` is not
    something it has, and a program of more than one source file is the
    code of each of them joined with the runtime. Which target a compiler
    was built for is what `nuocc --target` says, and the tests of a target
    whose programs the system cannot run are run by an emulator of its
    machine.

61. The Code of a Program Made Smaller (acwj Stage 64, `64_6809_Target`, the
    peephole optimiser). Once the whole of a 6809 program has been written,
    the code of it is put through a pass which looks at the few
    instructions standing at each place and leaves out the ones the machine
    has no need of. A value which is stored in a place and read straight
    back is the value the machine has in its register still, so the read is
    left out; the same is done for an address the machine has in X; and a
    jump to the label standing beside it is left out, since the machine
    would come to the same instruction by carrying on to it. The rules of
    the pass are written into the compiler rather than read from a file the
    way the tutorial has them, and a rule is a sequence of instructions
    together with what is left in their place, where a part of one written
    `%1` stands for any text and has to stand for the same text wherever
    the rule uses it. Nothing a program prints is changed by the pass, since
    what it takes away is what did nothing; what is changed is the code,
    which is smaller. Only the 6809 has the pass, because the rules are the
    instructions of that machine.

62. A Value of Two Bytes, Which Has a Sign (acwj Part 61, `61_What_Next`,
    the idea of adding `short`s). An integer of two bytes is written
    `short`, and a value of one is -32768 to 32767. Every machine this
    compiler builds for gives a short two bytes, whatever it gives an int,
    so it is the one type whose width is the same wherever a program is
    built. Reading a short gives the sign of the value back, and a value
    which is wider than a short is only stored in one behind a cast, which
    keeps the two bytes such a value can hold, so
    `short s= (short)70000;` is given 4464. A short widens to an int or to
    a long where the types are mixed, and it may stand wherever any other
    integer may: a variable, a parameter, a member of a struct, an element
    of an array, or what a pointer points at. A literal of 256 to 32767 is
    a short, so that such a value may be stored where a short is wanted
    without a cast, and the negative of a char, which has no sign of its
    own, is worked out as a short as well, so `-7` may be stored in one.
    A short given a value which is too wide for it where it is declared is
    refused, the way a char is.

63. A Correction to the Step of a Pointer (a fault in the x86-64 back end,
    found by test56). Items 6 and 15 promise that an increment or a
    decrement of a pointer moves on by the room one value of what it points
    at takes. The x86-64 code did not: it changed the address by one byte
    whatever the pointer pointed at, so `int *p; p++` left `p` one byte
    further on rather than four. The fix works the new address out whole in
    a register and stores it back, so `p++`, `p--`, `++p` and `--p` step by
    the width of the pointed-to value on x86-64 the same way they already
    did on arm64, qbe and the 6809. A change of one, and a pointer to a
    char, still only touch the value where it stands, so the common case
    costs no more than before.

64. Functions of a Variable Number of Arguments (acwj Part 61,
    `61_What_Next`, the idea of supporting variadic functions). A
    declaration whose parameter list ends in `...` names the first few
    arguments of a function which takes more of them than that, as
    `int printf(char *format, ...);` does. A call to such a function may
    give more arguments than the function names, and each of the extra
    ones takes the type an argument is given when nothing says otherwise:
    a value narrower than an int is widened to one, and everything else,
    a long and a pointer among them, travels as it is. A call still gives
    at most six arguments, since every one of them travels the same way as
    a parameter of a function of the language. The `...` needs at least
    one parameter before it, a later declaration of the function has to
    agree about whether it takes a variable number of arguments, and a
    body may not be given to such a function, since the language has no
    way of reaching the arguments it does not name; a function of a
    variable number of arguments is therefore always one written
    elsewhere, and the machine's own rules for calling one are what the
    compiler follows. On x86-64 the values travel in the registers the ABI
    gives them, and the number of registers carrying a value with a
    fraction, which is none, is put in `%al`; on arm64 the arguments the
    function names travel in registers and the rest are put on the stack,
    which is where the library functions of that machine look for them.
    The QBE back end hands every argument over in a register, which is
    what the machine asks for only where the two agree, so the test of a
    call to such a function does not apply to it, and the 6809 has no
    library function of this kind at all.

65. The Loop Which Asks Last (acwj Part 61, `61_What_Next`, the idea of
    extending the recognised grammar). A loop may be written with the
    condition at the end rather than at the beginning, as
    `do statement while (condition);` is, and its body then runs once
    before the condition is asked at all: a loop whose condition is false
    from the first still runs its body one time, which is what tells it
    apart from a while loop. Every pass after the first begins only where
    the condition held. The body is one statement, so more than one of them
    is written between braces, and the semicolon which closes the loop
    belongs to the loop rather than to the body, since a body which is a
    statement ending in one would otherwise take it. A break written in
    the body leaves the loop at once, and a continue goes on to the
    condition rather than back to the body, since the condition is what
    says whether there is another pass to begin; a loop written within a
    loop is left by a break one loop at a time, as it is anywhere else.

66. An Array Given Its Values Where It Is Declared (acwj Part 61,
    `61_What_Next`, the idea of extending the recognised grammar). An
    array declared inside a function may now be given the values it starts
    with, which items 37 and 53 said could only be done for a global one:
    `int list[5]= { 1, 2, 3, 4, 5 };`. The values are stored into the
    elements one after another where the declaration stands, so each is
    worked out while the program runs and may be anything the program can
    work out there, `int counted[]= { size, size * 2 };` among them, and
    the size may be left out as it may for a global array. An element the
    array was not given a value for is given nothing rather than whatever
    its storage happened to hold, which is what the elements left over in
    a global array start out holding as well: a number of zero, and an
    address which points at nothing. A value which does not fit the type
    of an element is refused, as is a list with more values than the array
    has room for, and a member of a struct may not be given values at all,
    since the room a member takes is part of a value of the struct it
    stands in and there is nothing to put a value in until one exists.

67. The Peephole Optimiser Reaching a Second Machine (acwj Part 61,
    `61_What_Next`, the rest of the idea of optimising the code the
    generator wrote). Item 61 gave the pass to the 6809 alone, since a
    rule of it is a sequence of the instructions of one machine and is only
    right about the code that machine's generator writes. The pass is now a
    pass of the compiler which a target names its own rules for, and the
    arm64 and the x86-64 have them. A branch to the label standing next to
    it is taken away, since the machine comes to the same instruction by
    carrying on to it; what a return statement leaves is this, and it is
    much the commonest thing the pass takes away. A value of eight bytes, a
    long or an address, which is held in the place of a register and read
    straight back is left in the register, since a store from a register
    leaves the value there and the read of a value that wide brings back
    exactly what the register holds; what the code generator writes when an
    expression worked out and held for the next one has been put in its
    place is this. The arm64 takes away the read of a char as well, since a
    char has no sign of its own and a register holding one holds the whole
    of it; the x86-64 leaves that read alone, because a store of one byte
    names the register one way and a read of it names it another, and a
    rule can only say that the same text stands on two lines. A read of a
    narrower value which has a sign is left alone on both machines: such a
    read gives the value the width of its type back, and the register may
    hold the wider value the program worked out, so taking the read away
    would print something else for a program whose arithmetic ran past the
    width of its type. What the pass leaves is recorded beside the programs
    under test/arm64 and test/x86_64, and the suite compares what the
    compiler writes with it; every program of the suite still prints what
    it printed before, since what the pass takes away is what did nothing.

68. Arrays of Composite Values, and Stepping a Pointer to One (acwj Part
    61, `61_What_Next`, the rest of what items 27, 28, 29 and 53 left out).
    An array may now hold composite values: `struct point path[4];`
    declares room for four of them, at the top level of the program or
    inside a function, and an element is reached by counting the room a
    whole element takes along from the first one, so `path[3]` is the
    fourth element and its members are reached as `path[3].x` and
    `(&path[3])->y`. A pointer to a composite value is stepped on by that
    same room, so `p + 1`, `1 + p`, `p - 1`, `p += 2`, `p++` and `p= p + 1`
    all move a `struct point *` from one element of such an array to the
    next, `p--` and `--p` move it back, and `p[i]` reaches the element that
    many along. The room a composite takes comes from its definition rather
    than from its type value, so the definition travels beside the type
    wherever an offset into one is worked out; that is what lets an array of
    composite values be indexed and a pointer to one be stepped on.

    An array of composite values cannot be given values where it is
    declared, since there is no way of writing a composite value down: a
    list of values written for such an array is refused with a message
    saying so, and an element is stored into one member at a time or given
    a whole value later as item 69 describes. A local array of them is
    cleared where it is declared instead, the room of it
    being walked a byte at a time and every byte given a zero, which leaves
    an element which was not stored into holding the nothing an element of
    any other local array is left holding. A global array of them is given
    that same nothing by the room its declaration reserves. A whole
    composite value still cannot be read as a value, passed or returned, so
    `print path[0]` and `return (path[0]);` are still refused, while one
    place holding such a value may be given the contents of another as item
    69 describes.

69. A Whole Composite Value Given to a Place Holding One (acwj Part 61,
    `61_What_Next`, the rest of what items 27, 28, 29 and 68 left out). A
    place holding a composite value may now be given the contents of
    another place holding one of the same type, written with `=`:

        struct point path[4];
        struct point first;

        first= path[3];

    Both sides have to hold the same composite type, so a struct may not be
    given a union or a struct of another type, and what is given has to be
    a place holding such a value rather than a pointer to one or a number:
    the room is taken from what a place holds rather than from a value
    worked out, so `first= path[3]` copies, while `first= &path[0]` and
    `first= 3` are both refused. A member, an element of an array and a
    value reached through a pointer are all places, so `box.one= box.two`,
    `path[0]= path[1]` and `*to= *from` all copy, a value may be copied
    into a member of itself, and a value may be its own source.

    The room of the target is given over a piece at a time, each piece as
    wide as the alignment the two places share, since a value is moved as a
    whole and two places are lined up for a value of that width: a value of
    eight bytes which stands at eight bytes moves in one piece, while one
    of eight bytes which stands at four moves in two. The room a composite
    value takes is rounded up to its alignment when its type is built, so
    it is a whole number of those pieces and a copy misses no byte of the
    value and writes no byte outside it.

    The copy is a statement rather than a value. What it stores is nothing
    an expression may be given, so it cannot stand where a value is wanted:
    `print a= b`, `n= (a= b)`, `return (a= b);`, `a= b= c` and a copy
    written as the condition of a statement are all refused. A whole
    composite value still cannot be read as a value, passed to a function
    by value or returned by value.
