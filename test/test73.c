/*
 * A member written as an unnamed struct or union definition is an anonymous
 * member: the names of its own members are reached as though they stood in
 * the type which holds it, so one piece of room is given two views and a
 * member is written the same way whether it stands in the type itself or
 * inside an anonymous member of it. `item.number` and `pointer->number`
 * both reach a member which was never given a name of its own:
 *
 *     struct box {
 *         int tag;
 *         union { int number; char glyph; };
 *     };
 *
 * The room an anonymous member takes is the room its own type takes, lined
 * up the way any other member is, so the members of it stand inside the
 * room of the type which holds it and not beside it. Its members may not
 * share a name with any other member the type reaches, since a name written
 * for a value has to say which member is meant.
 *
 * A value written where one of these is declared has the members of an
 * anonymous member written among the members of the type which holds it, in
 * the order they stand, the way a list of values has always worked. A list
 * of its own may be written for one as well, when it is easier to read.
 */
struct point { int x; int y; };

struct box {
    char tag;
    union {
        int number;
        char glyph;
    };
    struct {
        int left;
        int right;
    };
    struct point corner;
};

union pair {
    struct { int first; int second; };
    int values[2];
};

struct text {
    struct { char word[8]; int size; };
};

struct deep {
    struct {
        struct { int leaf; };
        int branch;
    };
};

int main() {
    struct box item;
    struct box copy;
    struct box *pointer;
    union pair both;
    int total;

    pointer = &item;

    /* Every member is reached by its own name, whether it stands in the
     * type itself or inside an anonymous member of it. */
    item.tag = 1;
    item.number = 2;
    item.left = 3;
    item.right = 4;
    item.corner.x = 5;
    item.corner.y = 6;

    print item.tag;
    print item.number;
    print pointer->left;
    print pointer->right;
    print item.corner.x + item.corner.y;

    /* Two members of an anonymous member stand in the room each one takes,
     * so their addresses are not the same one. */
    print &item.left == &item.right;

    /* A whole value may be given to another place holding one of the same
     * type, anonymous members and all. */
    copy = item;
    print copy.number;
    print copy.right;
    print copy.corner.y;

    /* The members of an anonymous member are written among the members of
     * the type which holds them, in the order they stand. */
    struct box given = { 7, 8, 9, 10, { 11, 12 } };
    print given.tag;
    print given.number;
    print given.left + given.right;
    print given.corner.x + given.corner.y;

    /* A list of its own may be written for an anonymous member as well. */
    struct box braced = { 13, { 14 }, { 15, 16 }, { 17, 18 } };
    print braced.tag;
    print braced.number;
    print braced.left + braced.right;
    print braced.corner.y;

    /* A value given to an anonymous member reachable through a name of the
     * type itself is read back the same way. */
    union pair one = { 20, 21 };
    union pair two = { { 22, 23 } };
    print one.first;
    print one.values[1];
    print two.second;

    /* The room of an anonymous union is the room of its largest member. */
    both.first = 30;
    both.second = 31;
    print both.values[0];
    print both.values[1];

    /* An array member of an anonymous member is reached as its own name,
     * and may be given a string where the value is declared. */
    struct text line = { "hi", 2 };
    print line.word[0];
    print line.word[1];
    print line.word[2];
    print line.size;

    /* An anonymous member may stand inside an anonymous member, and a
     * name at any depth is reached the same way. */
    struct deep down;
    down.leaf = 40;
    down.branch = 41;
    print down.leaf;
    print down.branch;

    /* A member reached this way is a place like any other: it can be
     * counted up or down, and it can be handed to a function. */
    total = 0;
    item.left++;
    item.right--;
    total = item.left + item.right;
    print total;

    return (0);
}
