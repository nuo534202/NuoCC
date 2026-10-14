#include <stdio.h>

int base= 7;

int main() {
    int x= 2000 + 3 + 4 * 5 + 6;

    /* An expression of numbers is worked out while the program is being
     * compiled, and it holds the same value it would have at run time. */
    print x;
    print 2 * 3 + 4;
    print (1 + 2) * (3 + 4);
    print 1 - 5;
    print 30 / 4;
    print 17 / 5 * 5;
    print 3000 + 1;
    print 200 + 100;

    /* The operators which are not arithmetic are worked out as well. */
    print 1 << 4;
    print 100 >> 2;
    print 6 & 3;
    print 6 | 3;
    print 6 ^ 3;
    print ~0;
    print !5;
    print !0;
    print -3 * -3;

    /* A number which does not fit an int is left for the program, and it
     * comes out the same either way. */
    print 2000000000 + 2000000000;
    print 1000 * 1000 * 1000;

    /* A condition, an argument and a switch are worked out too. */
    if (2 + 3 == 5) { print 1; }
    printint(4 * 5 + 1);
    print base + 2 * 3;

    switch (1 + 1) {
        case 2: { print 2; break; }
        default: { print 3; }
    }
    return(0);
}
