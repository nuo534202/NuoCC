# Composite Types: Design

This is the design part of the journey, the one the tutorial spends before
writing any code for structs, unions and enums. It records how they will be
represented in this compiler and what each of the stages which follow has to
do, so that the code which comes later is written against a decision rather
than worked out as it goes. Nothing in the compiler changes here: it behaves
exactly as it did before, and the regression suite is the evidence.

## What the Language Has To Support

A struct is a type whose members are laid out one after another, each at a
fixed offset from the base of the value:

```c
struct pair {
    int  first;
    char second;
};

struct pair item;

item.first= 4;
item.second= 'x';
```

A union is a type whose members all start at the same address, so writing one
member is reading another:

```c
union both {
    int  number;
    char letter;
};
```

Both may be written without a tag, and a struct or union member may itself be
an anonymous struct or union, which is how the tutorial's example puts a union
inside a struct without naming either of them. A member may also be a pointer
to the struct it stands in, which is what makes a linked list possible, so the
type of a member has to be able to refer back to a type which is not finished
yet.

An enum gives names to integer values, counting up from zero unless a value is
written out, and starting again from the last value written:

```c
enum { apple= 1, banana, carrot, pear= 10, peach };
```

The names are ordinary names for integer constants, so they are looked up the
same way a variable is. The tag, on the other hand, is not an ordinary name:
C allows a tag and a variable to share one, so `struct foo` and `int foo` may
be declared side by side. A tag which is reused as a *different* composite is
an error, and an enum value may not be reused as the name of a variable or of
another enum value.

## How They Will Be Represented

The pieces which already exist are enough to carry most of this.

`PrimitiveType` keeps the level of indirection in its bottom four bits and the
primitive type in the rest, so a struct adds a primitive type with its own
value and nothing else changes: a struct, a pointer to a struct and a pointer
to a pointer to a struct are one type each, put together the same way a char
and its pointers are.

`Symbol` grows what a composite needs and nothing more:

| Field | What it holds |
|---|---|
| `members` | the members of a struct or a union, in order |
| `member_offset` | where a member starts, from the base of the struct |
| `size` | how many bytes a value of the type takes |
| `value` | the number an enum value stands for |

A member is a symbol like any other, which is what lets a member be of any
type the language has, including a pointer back at its own struct: the
compiler already knows how to say "a value of this type" and how much room it
takes. `size` is also what `sizeof` will read when it arrives, and what a
variable of a composite type takes when one is copied.

A variable of a composite type has to reach the members of that type. The
symbol table holds one entry per composite tag, the way the tutorial's
separate list of struct and union types does, and a variable of that type
carries a shared pointer to the definition rather than a copy of it, so the
members stay reachable and the type stays one thing however many variables
are declared with it. The alternative the tutorial used, an index or a
pointer into a table, would be a second way of naming the same thing in a
language which already has one.

The name spaces are the one part of the design which the current symbol
table cannot express: `FindSymbol` looks a name up in one place, so a tag and
a variable of the same name would be one symbol. The table gains a second
name space for type tags when the first tag is declared, in the stage which
parses struct declarations. It is deliberately not added now, because there
is nothing yet to look up in it and no way to test it.

## What Changes in the Symbol Table, and What Does Not

The tutorial rewrites its symbol table here, from one array with the globals
at one end and the locals at the other into several singly linked lists, and
gives each function a list of its own parameters. It does that so that a
search for a global does not have to step over a function's parameters, and so
that the two ends of the array cannot meet.

This compiler does not need that rewrite. Its table is a vector which is
marked before a function's parameters are declared and restored to that mark
once the body has been parsed, so the parameters and the locals of a function
are gone from the table before the next function is read, and a global is
never confused with either. The parameters of a function are kept in its own
node, which is the tutorial's `member` list by another name. A linked list
would be more code to read and would give back nothing which the vector and
the marks do not already give.

What the rewrite is really about, for the stages which follow, is that names
of different kinds have to be told apart. That is the second name space
described above, and it will arrive with the first kind of name which needs
it.

## How a Member Is Read and Written

`item.second` is the value at a fixed distance from the base of `item`, which
is the same shape as the array indexing the compiler already does: an address,
an offset and a load sized by the type of what is at the other end. The
difference is that the offset is a number the parser counted rather than one
the program works out, so it can be part of the tree instead of an expression
which adds to it.

Reading a member and writing one are therefore two new node kinds, or one node
kind used in the two places an lvalue and an rvalue are already told apart.
The choice between them is small and belongs to the stage which implements
member access, once the shape of the offset arithmetic is in front of us.

## The Stages Which Follow

| Stage | What it does |
|---|---|
| 31 | parses `struct name { ... };`, its member offsets and its size |
| 32 | reads and writes a member, as `base.member` |
| 33 | unions: the members share an address, sized by the largest |
| 34 | enums, whose names stand for numbers, and `typedef` |

An anonymous struct or union member is left until the stage which needs it; the
member list described above is what it will hang from, so it is a change to
the parser rather than to the representation.
