#include <stddef.h>

int calls;

int counted(void) { calls= calls + 1; return(1); }

int main() {
    int x;
    int y;
    int z;
    int *pointer;

    /* A side which is not needed to know the answer is not worked out at
     * all, which the number of calls says. */
    calls= 0; z= 0 && counted(); print z; print calls;
    calls= 0; z= 1 || counted(); print z; print calls;
    calls= 0; z= 1 && counted(); print z; print calls;
    calls= 0; z= 0 || counted(); print z; print calls;

    /* What the two operations answer, for both values of each side. */
    for (x= 0; x <= 1; x= x + 1)
        for (y= 0; y <= 1; y= y + 1) {
            z= x && y;
            print z;
        }

    for (x= 0; x <= 1; x= x + 1)
        for (y= 0; y <= 1; y= y + 1) {
            z= x || y;
            print z;
        }

    /* The side which is not worked out may be the one which would go
     * wrong: a pointer which points at nothing is not read through. */
    pointer= NULL;
    if (pointer && *pointer == 1) { print 1; } else { print 2; }

    /* The answer is a value like any other, so it may stand in a larger
     * expression, be chained or be the condition of a decision. */
    print (0 && 1) + (1 || 0);
    print (1 && 1 && 0) + (0 || 0 || 1);
    print (1 && 1 && 1) + (0 || 0 || 0);
    z= (2 && 3) ? 4 : 5;
    print z;
    return(0);
}
