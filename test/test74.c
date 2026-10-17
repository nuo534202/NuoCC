/*
 * A value of a composite type may be given where a global variable is
 * declared, the same way a value of any other type may. The value is
 * written as the values of the members of the type, between braces, which
 * is one way a composite value is ever written:
 *
 *     struct point origin = { 0, 0 };
 *
 * The values are worked out while the program is being compiled, since the
 * compiler is what puts them in the storage of the variable, so a value
 * which the program works out as it runs may not stand here. The room of a
 * member which is not given a value starts out at zero, the way an element
 * of an array which is not given one does, and the braces may be closed
 * before every member has been written for.
 *
 * A member may be a composite value of its own, which takes a list of its
 * own, or an array, which takes a list of element values or the characters
 * of a string when it holds characters. A member which is a pointer to a
 * char is given a string, and holds the address of it the way such a value
 * does anywhere else. A union takes the value written for its first member,
 * since its members share one piece of room, and the members of an
 * anonymous member are written among the members of the type which holds
 * it.
 */
struct point { int x; int y; };
struct span { struct point from; struct point to; };
struct text { char word[8]; int size; };
struct named { char *name; int value; };
struct mixed { char tag; long wide; short narrow; };
struct box {
    int tag;
    union { int number; char glyph; };
    struct { int left; int right; };
};
union both { int number; char text[4]; };

struct point origin = { 0, 0 };
struct point one = { 1 };
struct point none = {};
struct span bridge = { { 1, 2 }, { 3, 4 } };
struct text word = { "hi", 2 };
struct named item = { "hello", 7 };
struct mixed held = { 1, 2, 3 };
struct box given = { 1, 2, 3, 4 };
struct box braced = { 5, { 6 }, { 7, 8 } };
union both shared = { 9 };
struct point worked = { 1 + 2, 4 * 5 };
struct point negative = { -3, 4 };
int numbers[4] = { 9, 8 };
short halves[3] = { 4, 5 };

int main() {
    /* Every member is written for, in the order the definition gives. */
    print origin.x;
    print origin.y;

    /* The members left over start out at zero, and `{}` leaves them all. */
    print one.x;
    print one.y;
    print none.x + none.y;

    /* A member which is a composite value of its own takes a list of its
     * own. */
    print bridge.from.x + bridge.from.y;
    print bridge.to.x + bridge.to.y;

    /* An array member takes a list of values, or the characters of a
     * string when it holds characters; the zero byte it ends with is one
     * element more, and the elements left over start out at zero. */
    print word.word[0];
    print word.word[1];
    print word.word[2];
    print word.size;

    /* A member which is a pointer to a char is given a string, and holds
     * the address of it. */
    print item.name[0];
    print item.name[4];
    print item.value;

    /* The room a member stands at is lined up for it, so the value of a
     * long member stands after the room a char member takes and the room
     * which lines it up. */
    print held.tag;
    print (int) held.wide;
    print held.narrow;

    /* The members of an anonymous member are written among the members of
     * the type which holds it, and a list of their own may be written for
     * one as well. */
    print given.tag;
    print given.number;
    print given.left + given.right;
    print braced.tag;
    print braced.number;
    print braced.left;
    print braced.right;

    /* A union takes the value written for its first member. */
    print shared.number;

    /* The values are worked out while the program is compiled. */
    print worked.x + worked.y;
    print negative.x;

    /* A global array of a plain type is written the way it always was, and
     * an element of one which takes two bytes takes two bytes of the
     * storage rather than the word of the machine it stands on. */
    print numbers[0] + numbers[1] + numbers[2];
    print halves[0];
    print halves[1];
    print halves[2];

    return (0);
}
