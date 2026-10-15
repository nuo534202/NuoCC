/* The shapes which are easy to get subtly wrong: a value narrower than a
 * register, an address which comes from another file, and a dereference
 * which has to reach the element before the value it reads. */
extern int shared;
extern int values[3];

char *lines[3];
char **rows;

int negative(void) { return(-1); }

int add(int a, int b) { return(a + b); }

int main() {
    int i;
    int k;
    long wide;

    /* The dereference reaches the element first, so `*rows[1]` reads the
     * character `rows[1]` points at and not one of `rows[0]`'s. */
    lines[0]= "abc";
    lines[1]= "def";
    rows= lines;
    print *rows[1];
    print (*rows)[1];

    /* A value keeps its own value through every operation it takes part
     * in, whatever width the register holding it has. */
    i= 200;
    print i + 100;
    print i > 100;
    print i == 200;
    i= 300;
    print i - 300;
    print i / 100;
    print i % 100;

    wide= -8;
    if (wide < 0) { print 1; }
    if (wide / 2 == -4) { print 2; }
    if (wide * -1 == 8) { print 3; }
    print (int)wide * -1;

    k= negative();
    if (k < 0) { print 4; }
    if (k + 1 == 0) { print 5; }

    /* A cast takes a value of one type and leaves the bits of another,
     * read back as the number that type holds: what is left of a negative
     * number is a negative number. */
    wide= -8;
    print (int)wide;
    print (int)wide + 1;
    if ((int)wide < 0) { print 6; }
    print (char)wide;
    print (char)300;
    print add((int)wide, 10);
    print (int)wide == -8;
    print (char)-1;
    wide= -1000;
    print (int)wide / 10;
    print (int)(wide * 2);
    if ((long)300 == 300) { print 7; }

    /* A name whose storage is in another file is stored into and asked for
     * its address the same way any other global name is. */
    shared= 5;
    print shared;
    values[2]= 7;
    print values[2];
    return(0);
}
