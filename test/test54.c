#include <stddef.h>

int list[]= { 3, 5, 7, 9, 11, 13, 15 };
int *pointer;
int *nothing;

int main() {
    int step;
    int x;

    /* A cast inside a comparison inside a logical expression: the '==' has
     * to bind more tightly than the '||', whichever way each side of it is
     * written down. */
    x= 5;
    if ((char)5 == x || (char)6 == x) { print 1; }
    if ((long)5 == x && (char)6 != x) { print 2; }
    print ((char)5 == x) + ((char)5 == 6);

    /* One address may be compared with another, which is how a pointer is
     * asked whether it points at anything at all. */
    pointer= list;
    nothing= NULL;
    if (pointer == NULL || nothing == NULL) { print 3; }
    if (pointer != NULL || nothing != NULL) { print 4; }
    if (pointer != NULL && nothing != NULL) { print 5; } else { print 6; }
    print pointer < list + 1;

    /* An integer added to an address through a compound assignment is
     * scaled by the size of what the pointer points at, as it is with a
     * plain assignment. */
    pointer= list;
    print *pointer;
    pointer += 1;
    print *pointer;
    pointer += 2;
    print *pointer;
    pointer -= 1;
    print *pointer;

    step= 3;
    pointer= list;
    pointer += step;
    print *pointer;
    pointer -= step;
    print *pointer;
    return(0);
}
