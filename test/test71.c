/*
 * A goto jumps to a label, which is a name written in front of a statement
 * and which a goto reaches from anywhere in the same function. Nothing new
 * is needed to work one out: a goto is a jump to a label and the label is
 * where that label stands, which is what a while loop is written with as
 * well.
 *
 * A label belongs to the function it is written in, so two functions may
 * each give a name of their own, and a goto may name a label which is
 * written after it, since the whole body is read before the code is
 * written.
 */
int counted(void) {
    int i;

    i = 0;
again:
    i = i + 1;
    if (i < 4) goto again;

    return (i);
}

int main() {
    int i;
    int j;
    int total;

    /* A goto leaves a loop from the inside, whatever depth it stands at. */
    for (i = 0; i < 5; i = i + 1) {
        for (j = 0; j < 5; j = j + 1) {
            if (i == 2 && j == 2) goto left;
            print i * 10 + j;
        }
    }
left:
    print 77;

    /* A goto written backwards is a loop of its own. */
    i = 0;
back:
    print i;
    i = i + 1;
    if (i < 3) goto back;

    /* A goto reaches a label inside a switch as well. */
    total = 0;
    i = 0;
top:
    i = i + 1;
    switch (i) {
        case 1: { total = total + 1; goto top; }
        case 2: { total = total + 2; break; }
        default: { total = total + 8; }
    }
    print total;

    /* A label may name a statement which does nothing, and a goto may jump
     * over what stands between it and the label. */
    goto nothing;
    print 1;
nothing: ;

    /* The label of a function is its own, so this one counts to four
     * without reaching anything of main. */
    print counted();

    return (0);
}
