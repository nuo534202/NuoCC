int list[]= { 3, 5, 7, 9, 11, 13, 15 };
char letters[]= { 1, 2, 3 };
long numbers[]= { 10, 20, 30 };
int *pointer;
char *small;
long *wide;
int a;

int main() {
    /* An increment or a decrement of a pointer moves on to the next value
     * of what it points at, whatever the size of that value is, and both
     * ways of writing it and both readings of it work. */
    pointer= list;
    print *pointer;
    pointer++;
    print *pointer;
    pointer++;
    print *pointer;
    pointer--;
    print *pointer;
    ++pointer;
    print *pointer;
    --pointer;
    print *pointer;
    print *(pointer++);
    print *pointer;

    small= letters;
    small++;
    print *small;
    small--;
    print *small;

    wide= numbers;
    wide++;
    if (*wide == 20) { print 1; }
    wide--;
    if (*wide == 10) { print 2; }

    /* What is left over once one number has been taken from another as many
     * whole times as it holds, written `%`, and the `%=` which stores it. */
    a= 24;
    print 24 % 9;
    print a % 5;
    print 2 + 24 % 9 * 2;
    print -24 % 9;
    a= 31; a %= 11; print a;
    if (100 % 7 == 2) { print 3; }
    return(0);
}
