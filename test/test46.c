#include <stdio.h>

int y= 3;
int x;
char *truth= "yes";
char *lie= "no";

int calls= 0;

int counted(void) {
    calls = calls + 1;
    return(7);
}

int main() {
    /* The condition chooses which of the two values is worked out. */
    x= 1 ? 10 : 20;
    print x;
    x= 0 ? 10 : 20;
    print x;

    /* It is an expression, so it may stand inside another one. */
    print 23 * (1 ? 3 : 4) - 18;
    print 23 * (0 ? 3 : 4) - 18;

    /* The value which is not taken is not worked out at all. */
    calls= 0;
    x= 1 ? 5 : counted();
    print x;
    print calls;
    x= 0 ? counted() : 5;
    print x;
    print calls;

    /* The two values agree on a type, and a pointer may be one of them. */
    print truth[0];
    print (1 ? truth : lie)[1];
    print 0 ? 'a' : 4000;

    /* Two calls which answer with nothing may be the two values, in which
     * case the whole expression answers with nothing too. */
    printint(1 ? 11 : 22);
    printchar('\n');
    printint(0 ? 11 : 22);
    printchar('\n');

    /* Nested, and taken to the right: the second '?' belongs to the value
     * taken when the first condition does not hold. */
    for (y= 0; y < 10; y= y + 1) {
        x= (y < 4) ? y + 2 : (y > 7) ? 1000 : y + 9;
        print x;
    }
    return(0);
}
