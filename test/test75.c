/*
 * An array of composite values may be given its values where it is
 * declared, one value for each element, which is what items 66, 73 and 77
 * gave every other kind of value and left this one out of:
 *
 *     struct point ends[2]= { { 1, 2 }, { 3, 4 } };
 *
 * A value written for an element is written one of the two ways a composite
 * value ever is: as the values of the members of the element type, between
 * braces, or as another place holding a value of the same type, which is
 * copied whole where the element stands. A global array is given values the
 * compiler works out, since it is the compiler which puts them in the
 * storage; a local one is given values the program works out as the
 * declaration is reached, so a copy of a place is given to a local array and
 * not to a global one.
 *
 * An array written without a size holds exactly the elements it was given,
 * and one with room for more keeps that room: an element which is not given
 * a value starts out holding nothing, which is the same nothing an element
 * of any other array which is not given a value holds. An element of such an
 * array is a place like any other afterwards, so it can be read, stored into,
 * counted up, and copied out whole.
 */
struct point { int x; int y; };
struct text { char word[8]; int size; };
struct box {
    int tag;
    union { int number; char glyph; };
};

struct point ends[2] = { { 1, 2 }, { 3, 4 } };
struct point path[] = { { 5, 6 }, { 7, 8 }, { 9, 10 } };
struct point rest[3] = { { 11, 12 } };
struct text words[2] = { { "hi", 1 }, { "ok", 2 } };
struct box boxes[] = { { 1, 2 }, { 3, 4 } };

int main() {
    struct point source;
    struct point few[2] = { { 24 } };
    struct point local[2] = { { 40, 41 }, { 42, 43 } };
    struct point taken;

    /* The values of the elements are the ones written for them, in the
     * order they are written. */
    print ends[0].x;
    print ends[0].y;
    print ends[1].x;
    print ends[1].y;

    /* An array written without a size holds exactly those elements. */
    print path[0].x;
    print path[1].y;
    print path[2].x;

    /* An element which is not given a value starts out at zero. */
    print rest[0].x;
    print rest[1].x + rest[1].y + rest[2].x;

    /* A member which is an array of char may be given a string. */
    print words[0].word[1];
    print words[1].word[0];
    print words[1].size;

    /* The members of an anonymous member are written among the members of
     * the element type. */
    print boxes[0].tag;
    print boxes[1].number;

    /* A local array is given its values where the declaration stands, and
     * an element left over starts out at zero there as well. */
    print local[0].x;
    print local[1].y;
    print few[0].x;
    print few[0].y;
    print few[1].x;

    /* A value written for an element may be another place holding one of
     * the same type, which is copied whole where the element stands. */
    source.x = 30;
    source.y = 31;
    struct point copies[2] = { source, source };
    print copies[0].x + copies[1].y;

    /* The element holds what was copied into it even after the place it was
     * copied from changes. */
    source.x = 50;
    print copies[0].x;

    /* An element is a place like any other once the array is declared. */
    ends[0].x = 60;
    ends[0].x++;
    print ends[0].x;
    taken = ends[1];
    print taken.y;
    taken = local[0];
    print taken.x + taken.y;

    return (0);
}
