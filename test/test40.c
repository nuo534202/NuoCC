enum numbers { low= -1, zero, five= 5, six };

int x;
int y;
int *p;
int box[4];
char letter= 'A';
int table[3]= { -1, 0, 1 };
char byte= (char)-1;

int main() {
    /* The four compound assignments. */
    x= 3; y= 15; y += x; print y;
    x= 3; y= 15; y -= x; print y;
    x= 3; y= 15; y *= x; print y;
    x= 3; y= 15; y /= x; print y;

    /* A compound assignment is an expression, and its value is the one it
     * stored. */
    x= 1;
    print x += 4;
    print x;

    /* It takes what is on its right first, like every assignment. */
    x= 1; y= 2;
    x += y += 3;
    print x;
    print y;

    /* A char, and a value reached through a pointer. */
    letter += 1;
    print letter;

    x= 7; p= &x;
    *p += 5;
    print x;
    p[0] *= 2;
    print x;

    /* The address of the target is worked out once, so an index which
     * counts is stepped once. */
    x= 0;
    box[0]= 100;
    box[x++] += 1;
    print box[0];
    print x;

    /* Negative values where a constant is wanted. */
    print low;
    print zero;
    print six;
    print table[0];
    print byte;

    switch (six) {
        case 6: { print 60; break; }
        case -6: { print 61; break; }
    }

    x= -1;
    switch (x) {
        case -1: { print 70; break; }
        default: { print 71; }
    }
    return(0);
}
