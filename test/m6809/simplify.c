/*
 * The code this target writes for this program is recorded beside it, in
 * simplify.s, and the suite compares what the compiler writes with what is
 * recorded. That is how the pass over the tree is tested: it is not allowed
 * to change what a program prints, so the code it leaves is the only thing
 * which says whether it works.
 *
 * Every operation the pass has a rule for stands here, and so do the
 * operations it has to leave as they are. A multiplication by a power of
 * two is written as a shift of the other side; an operation which does
 * nothing with one of its sides is taken away; and an operation which
 * answers with the number instead of the other side, such as `value & 0`,
 * is left as it was, since taking it away would take the other side away
 * with it.
 *
 * An operation whose other side is a call is left as it is as well when the
 * call is the side the pass would take away with the operation, and is
 * taken away when it is not: `bump() * 1` is the call alone, while
 * `bump() * 0` is still worked out.
 */
int value;
int count;

/* Every call of this one is seen in `count`, which is what the program
 * prints at the end. */
int bump() {
    count = count + 1;
    return (3);
}

void main() {
    value = 5;
    count = 0;

    /* A multiplication by a power of two, with the number on either side. */
    print value * 2;
    print value * 8;
    print 2 * value;

    /* An operation which does nothing with the side it is not on. */
    print value + 0;
    print 0 + value;
    print value - 0;
    print value * 1;
    print 1 * value;
    print value / 1;
    print value | 0;
    print 0 ^ value;
    print value >> 0;

    /* What is left alone: a division and a remainder, which do not answer
     * the way a shift would; a number taken from nothing; a multiplication
     * by nothing; and an operation which answers with the number. */
    print value / 2;
    print value % 2;
    print 0 - value;
    print value * 0;
    print value & 0;

    /* A call which stands beside a number which does nothing is not taken
     * away with the operation, and one which is the whole of the other side
     * is what is left. */
    print bump() * 0;
    print 1 * bump();
    print count;
}
