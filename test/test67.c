/*
 * The optimiser writes some operations another way, so what a program
 * prints is what says whether the rewritten form gives the value the
 * operation would have given. Every form the pass has a rule for is worked
 * out here, and so is every form it has to leave alone.
 *
 * An operation which does nothing with one of its sides is taken away, but
 * only where taking it away does not take the other side with it: a call
 * which prints something is never made to go away by writing a number
 * beside it which the answer does not depend on. What the calls here
 * counted is printed, so a call which was taken away shows up as a number
 * which is too small.
 */
int count;

int bump() {
    count = count + 1;
    return (3);
}

int main() {
    int value;
    long wide;
    char small;

    value = 7;
    wide = 1000;
    small = 100;

    /* A multiplication by a power of two is written as a shift. */
    print value * 2;
    print value * 8;
    print 2 * value;
    print (int)(wide * 4);
    print small * 2;

    /* An operation which does nothing with its other side is taken away. */
    print value + 0;
    print 0 + value;
    print value - 0;
    print value * 1;
    print 1 * value;
    print value / 1;
    print value | 0;
    print 0 | value;
    print value ^ 0;
    print value << 0;
    print value >> 0;

    /* What is left as it is: a division and a remainder, which do not
     * answer the way a shift would; nothing taken from a value; a
     * multiplication by nothing; and an operation which answers with the
     * number rather than with the other side. */
    print value / 2;
    print value % 2;
    print 0 - value;
    print value * 0;
    print value & 0;

    /*
     * A call standing beside a number which does nothing is still made
     * whether the operation is taken away or left as it is, and how many
     * of them were made is what the count says.
     */
    count = 0;
    print bump() * 0;
    print count;
    print bump() * 1;
    print count;
    print 0 + bump();
    print count;

    /* The same forms with a value below nothing, whose answer a shift
     * gives as well, since moving a value up and doubling it are the same
     * thing. */
    value = -7;
    print value * 2;
    print value * 4;
    print value / 2;
    print value % 2;
    print 0 - value;
    print value + 0;

    return (0);
}
