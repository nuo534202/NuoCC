/*
 * A value of a composite type may be given its value where it is declared,
 * the same way a value of any other type may. A composite value cannot be
 * read as a value, so what it is given is written one of the two ways a
 * composite value ever is: as the values of the members of its type, which
 * stand between braces, or as another place holding a value of the same
 * type, which is copied whole.
 *
 * The members are written in the order the definition gives them, and the
 * braces may be closed before every one of them has been written for: a
 * member which is not given a value starts out holding nothing, which is
 * the same nothing an element of an array which is not given a value
 * starts out holding. A member which is a composite value of its own takes
 * a list of its own between braces, and a member which is an array takes a
 * list of element values, or the characters of a string when it holds
 * characters. A union takes the value written for its first member, since
 * the members of a union share one piece of room.
 *
 * The values written may be worked out when the program runs, since the
 * declaration stands where the program is, and not only where the whole
 * program is read.
 */
struct point { int x; int y; };
struct span { struct point from; struct point to; };
struct line { struct point from; struct point to; char tag[8]; };
struct table { int nums[3]; };
union number { int whole; char digits[4]; };

int main() {
    int a;

    /* Every member is written for, in the order the definition gives. */
    struct point p = { 3, 4 };
    print p.x;
    print p.y;

    /* A member which is not given a value starts out at zero. */
    struct point one = { 9 };
    print one.x;
    print one.y;

    /* No value at all leaves the whole value at zero. */
    struct point none = {};
    print none.x;
    print none.y;

    /* A member which is a composite of its own takes a list of its own. */
    struct span s = { { 1, 2 }, { 3, 4 } };
    print s.from.x + s.from.y + s.to.x + s.to.y;

    /* A member which is an array takes a list of values of its own. */
    struct table t = { { 5, 6, 7 } };
    print t.nums[0] + t.nums[1] + t.nums[2];

    /* An array of char may be given a string, and it is the only member of
     * the value which is: the string fills the array and the members
     * written after it follow on. */
    struct line l = { { 1, 2 }, { 3, 4 }, "hi" };
    print l.from.x;
    print l.to.y;
    printchar(l.tag[0]);
    printchar(l.tag[1]);
    printchar('\n');
    print l.tag[2];

    /* A union takes the value written for its first member. */
    union number n = { 65 };
    print n.whole;

    /* A value may be given another place holding one of the same type,
     * which is copied whole, members and all. */
    struct point q = p;
    print q.x;
    print q.y;
    struct line m = l;
    print m.from.x;
    print m.to.y;
    print m.tag[0];
    print m.tag[1];

    /* The values written may be worked out when the program runs. */
    a = 11;
    struct point r = { a, a + 1 };
    print r.x;
    print r.y;

    return (0);
}
