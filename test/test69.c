/*
 * The compound assignments which combine their two sides with a bitwise or
 * a shift operator: `&=`, `|=`, `^=`, `<<=` and `>>=`. Each stores the
 * result back in the target and yields it, the way the arithmetic compound
 * assignments do, and the operators themselves are the ones the compiler
 * already had.
 *
 * What may stand on the left is what an assignment may store into, so a
 * variable, a place a pointer reaches, an element of an array and a member
 * of a value all work. What stands on the right is worked out first, so two
 * of them written one after the other read from the right, and a value
 * which does not fit the target is refused the same way an arithmetic
 * compound assignment refuses one.
 */
struct flags {
    int bits;
};

int main() {
    int x;
    int y;
    int list[3];
    int *p;
    long wide;
    struct flags box;

    /* Each operator stores the new value and yields it. */
    x = 12;
    print x &= 10;
    print x;
    x |= 5;
    print x;
    x ^= 9;
    print x;
    x <<= 3;
    print x;
    x >>= 2;
    print x;

    /* A place a pointer reaches, an element and a member all work. */
    list[1] = 6;
    list[1] &= 3;
    print list[1];
    p = &list[2];
    *p = 5;
    print *p <<= 2;
    print list[2];
    box.bits = 7;
    print box.bits ^= 3;

    /* What stands on the right is worked out first, so two of them written
     * one after the other read from the right. */
    y = 1;
    x = 30;
    x &= y |= 4;
    print x;
    print y;

    /* A value of another width takes them as well. */
    wide = 1;
    wide <<= 5;
    print wide == 32;
    wide >>= 3;
    print wide == 4;

    return (0);
}
