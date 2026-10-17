/*
 * A whole composite value may be stored into a place which holds one of
 * the same type. What the place holds is given the contents of the other
 * one rather than a value being worked out, so the two have to be of the
 * same type and the one given has to be a place holding one.
 *
 * The room of the target is given over a piece at a time, each piece as
 * wide as the alignment the two places share, since a value is moved whole
 * and the two places are lined up for a value of that width. The room of a
 * composite value is a whole number of those pieces, so a copy misses no
 * byte of the value and writes no byte outside it, which is what the
 * elements standing next to the ones copied over are here to show.
 */
struct three {
    char a;
    char b;
    char c;
};

struct four {
    char first;
    short second;
};

struct eight {
    int x;
    int y;
};

struct wide {
    long one;
};

struct holder {
    struct eight one;
    struct eight two;
};

/* A member may be an array of composite values, whose room is part of the
 * room of the value holding it. */
struct row {
    char tag;
    struct three cells[2];
};

union same {
    int number;
    char bytes[4];
};

struct three triples[3];
struct eight eights[2];
struct wide wide_global;
struct holder box;
struct row rows[2];
union same shared;
union same other;

/* Give one whole value what another holds, through the pointers the
 * function was given. */
void fill(struct holder *to, struct holder *from) {
    *to = *from;
    return;
}

int main() {
    struct three small;
    struct four four_bytes;
    struct eight local;
    struct wide wide;
    struct holder a;
    struct holder b;
    struct holder *p;
    struct holder *q;

    /*
     * Every element of an array is filled first, so that a copy of one of
     * them which ran over its own room would show up in the element next
     * to it.
     */
    triples[0].a = 1;  triples[0].b = 2;  triples[0].c = 3;
    triples[1].a = 11; triples[1].b = 12; triples[1].c = 13;
    triples[2].a = 21; triples[2].b = 22; triples[2].c = 23;

    eights[0].x = 10; eights[0].y = 20;
    eights[1].x = 30; eights[1].y = 40;

    /* A value lined up to one byte is given over a byte at a time. */
    triples[2] = triples[0];
    print triples[2].a;
    print triples[2].b;
    print triples[2].c;
    print triples[1].a;
    print triples[1].c;

    /* A value wider than the alignment it stands at is given over in more
     * than one piece. */
    local = eights[1];
    print local.x;
    print local.y;
    print eights[0].x;

    /* A local value and a global one are the same kind of place. */
    wide.one = 1234;
    wide_global = wide;
    wide = wide_global;
    print (int)wide.one;

    /* A value whose members stand at two widths of their own. */
    four_bytes.first = 5;
    four_bytes.second = 600;
    small = triples[1];
    print four_bytes.first;
    print four_bytes.second;
    print small.b;

    /* A member of a composite value is a place holding one too. */
    a.one.x = 1;  a.one.y = 2;  a.two.x = 3;  a.two.y = 4;
    b.one.x = 7;  b.one.y = 8;  b.two.x = 9;  b.two.y = 10;

    box = a;
    print box.one.x;
    print box.one.y;
    print box.two.x;
    print box.two.y;

    a.one = b.two;
    print a.one.x;
    print a.one.y;
    print a.two.y;

    /* Reaching a whole value through a pointer reaches what it points at,
     * so the copy gives that place the contents of the other one. */
    p = &a;
    q = &b;
    *q = *p;
    print q->one.x;
    print q->two.y;

    fill(&b, &a);
    print b.one.x;
    print b.two.x;

    /* A value may be its own source: the room is given what it holds. */
    a.one = a.one;
    print a.one.x;
    print a.one.y;

    /*
     * A member array of composite values stands inside the value holding
     * it, so the room of the whole value is what a copy gives over and
     * the elements of the array are reached in the target afterwards.
     */
    rows[0].tag = 1;
    rows[0].cells[0].a = 2;
    rows[0].cells[0].b = 3;
    rows[0].cells[0].c = 4;
    rows[0].cells[1].a = 5;
    rows[0].cells[1].b = 6;
    rows[0].cells[1].c = 7;
    rows[1].tag = 9;
    rows[1].cells[0].a = 90;

    rows[1] = rows[0];
    print rows[1].tag;
    print rows[1].cells[0].a;
    print rows[1].cells[1].c;

    /*
     * A union takes the room of its widest member, and that whole room is
     * what is given over. The bytes are what every machine holds the same
     * way: which end of an int a byte stands at is the machine's own
     * business, so a union is read back here a byte at a time.
     */
    shared.number = 0;
    shared.bytes[0] = 7;
    shared.bytes[1] = 8;
    shared.bytes[2] = 9;
    shared.bytes[3] = 10;

    other = shared;
    print other.bytes[0];
    print other.bytes[1];
    print other.bytes[2];
    print other.bytes[3];

    return (0);
}
