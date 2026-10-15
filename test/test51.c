/* An expression which needs more values standing at once than there are
 * registers has the ones it is not using set aside until they are needed
 * again. */
int a= 1;
int b= 2;
int c= 3;
int d= 4;
int e= 5;
int f= 6;
int g= 7;
int h= 8;

int add(int x, int y) { return(x + y); }

int deep(int p1, int p2, int p3, int p4, int p5, int p6) {
    return(p1 + (p2 + (p3 + (p4 + (p5 + p6)))));
}

int main() {
    int i;
    int total;
    int x;

    x= a + (b + (c + (d + (e + (f + (g + h))))));
    print x;

    /* A call in the middle of such an expression keeps the values which
     * were worked out before it. */
    x= 3 * add(2, 3) - 5 * add(4, 6);
    print x;

    x= a + (b + (c + add(d, add(e, f)))) * (g + h);
    print x;

    /* What is set aside is done with at the end of the statement it was
     * set aside in. */
    total= 0;
    for (i= 0; i < 3; i= i + 1)
        total= total + (a + (b + (c + (d + (e + (f + (g + (h + i))))))));
    print total;

    /* The same inside a function of its own, with the values arriving as
     * parameters. */
    print deep(a, b, c, d, e, f);
    return(0);
}
