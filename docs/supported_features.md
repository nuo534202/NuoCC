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
    same is allowed for the variables declared inside a function. Every
    name of the list carries its own type: the `*`s and the array size
    written with a name are what give it a pointer or an array on top of
    the base type, so `char *b, c;` declares a pointer and a plain `char`.
    A name has to be declared before it is used, so a function can only
    reach the global variables declared above it. The variables are stored
    one after another in the order they are declared.
15. Pointer Arithmetic. An integer may be added to or subtracted from a
    pointer, and is scaled by the size of what the pointer points at, so
    `&c + 1` is the address of the next value of `c`'s type and not the
    address one byte along. Nothing else may be done with a pointer: it
    cannot be multiplied, divided or compared yet, and its address cannot
    be printed.
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
    declares an array and a plain variable. The address of the whole array
    is not a value which may be stored into, so `a= p;` and `a++;` are
    refused. One dimension is known so far; a size which is zero, is not a number, or
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

    A struct value cannot be read, written, passed to a function or returned
    as a whole yet. Members can be read and written as described below.
    An array of structs, arithmetic on a pointer to a struct, a struct which
    contains itself by value, a struct with no members, and a
    tag which is defined twice are all refused with a message saying so.
    Pointers to different struct types are distinct even when their members
    have the same names and layout.

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
    Incrementing a member or a struct pointer, taking the address of an
    entire array member, arrays of structs, arithmetic on a struct pointer
    and copying a whole struct remain unsupported.

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
    whole composite value still cannot be copied, passed by value or
    returned, and arrays of composite values and arithmetic on pointers to
    them are still refused.

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
    file and line the text after them came from begin with `#`, and the
    scanner steps over them.

    `extern` declares a name whose storage is given in another source
    file, so the name may be used here but no storage is emitted for it.
    It has to come before the type, as in `extern int shared;`, at the top
    level of the program or inside a function. A name declared `extern` is
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
    or the values it starts with where it is declared. A `char`, an `int`
    or a `long` is given a number which has to fit it, so `int x= 2;` and
    `long big= 100000;` are taken while `char c= 300;` is refused. A
    variable of type `char *` is given a string: its characters are put in
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
    has room for, a value which is not a literal, an `extern` name given a
    value, a local array given a list of values, and an array written with
    neither a size nor values are all refused with a message saying so.

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
    of the value it was given, and a store keeps the bits which fit the
    storage. A pointer is given a number only behind a cast, which is how a
    pointer which points at nothing is written: `char *p= (void *)0;`. The
    header files define `NULL` as `(void *)0`, so `int *p= NULL;` and `char
    *text= NULL;` say the same thing. A cast to a struct, a union or void, a
    cast whose type does not agree with the variable it is given to, and a
    number given to a pointer without a cast are all refused with a message
    saying so.

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
