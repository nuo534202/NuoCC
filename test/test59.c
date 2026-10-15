/* A struct which holds a struct of its own. The name in front of the '.'
 * is worked out first, and the offset of the member inside the thing it
 * named is added to the offset which has already been worked out, so a
 * member of a member is reached through two offsets rather than through
 * one name. */
struct inner {
    int  age;
    char tag;
};

struct outer {
    char first;
    struct inner in;
    long wide;
};

/* A union is as wide as its largest member and stands at the alignment of
 * the widest one, whether it is a global of its own or a member of a
 * struct. */
union packed {
    int  number;
    char bytes[4];
};

/* The size of a member array is an expression, which the compiler works
 * out while it is compiling the program. */
struct holder {
    char label;
    union packed value;
    int  table[2 * 3];
};

struct outer thing;
struct outer *pointing;
struct holder box;

int main() {
    struct outer local;
    int i;

    /* A global struct, whose members are read and written through their
     * own names. */
    thing.first= 3;
    thing.in.age= 30;
    thing.in.tag= 'a';
    thing.wide= 3000;
    print thing.first;
    print thing.in.age;
    print thing.in.tag;
    print (int)thing.wide;

    /* The room a struct takes is worked out from the members it holds, a
     * struct of its own included, each lined up at its own alignment. */
    print sizeof(struct inner);
    print sizeof(struct outer);
    print sizeof(thing);

    /* A local struct stands in the frame beside the other locals, and its
     * members are reached the same way. */
    local.first= 4;
    local.in.age= 40;
    local.in.tag= 'b';
    local.wide= 4000;
    print local.in.age;
    print local.in.tag;
    print (int)local.wide;

    /* The thing in front of the '.' may be an address which is already
     * worked out, which is what a member of a struct and a pointer to one
     * both are. */
    pointing= &thing;
    print pointing->in.age;
    print (*pointing).in.age;
    print (&local)->in.tag;
    print (int)(&thing)->wide;

    /* A union which is a member of a struct shares its room with the other
     * members of the union, and the members of the struct stand at their
     * own alignments. */
    print sizeof(union packed);
    print sizeof(struct holder);
    print sizeof(box);

    box.label= 'z';
    box.value.number= 66051;
    print box.label;
    print box.value.bytes[0];
    print box.value.bytes[1];
    print box.value.bytes[2];
    print box.value.bytes[3];

    /* A member array whose size is an expression holds as many elements as
     * the expression says. */
    for (i= 0; i < 6; i= i + 1)
        box.table[i]= i * 7;
    print box.table[0];
    print box.table[5];
    print box.table[2] + box.table[3];
    return(0);
}
