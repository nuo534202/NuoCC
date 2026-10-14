int base= 4;
char *greeting= "global";

int main() {
    int x= 2, y= base + 3;
    int z= 5 * x - y;
    char *text= "Hello";
    char *local= greeting;
    int a= 1;
    char c= 'A';
    long w= 1000;
    int *p= &a;
    int i= 0;
    int total= 0;

    print x;
    print y;
    print z;
    print text[0];
    print local[0];
    print c;
    print *p;
    print p[0];

    /* A long is compared rather than printed, as printing narrows it. */
    if (w == 1000) { print 1; }

    /* The value is stored where the declaration stands, so it is worked
     * out again every time the statement is reached. */
    for (i= 0; i < 3; i= i + 1) {
        int step= i * 2 + 1;
        total= total + step;
    }
    print total;

    /* A local given no value holds whatever its storage held, so a value
     * is stored in it before it is read. */
    {
        int spare;
        spare= 42;
        print spare;
    }
    return(0);
}
