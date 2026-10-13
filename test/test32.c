#include <stddef.h>
#include <stdio.h>
#include <stdio.h>  /* the include guard keeps this from being read twice */

/*
 * The pre-processor takes this comment away, and expands the two
 * macros below, before the compiler ever sees the program.
 */

#define START 4
#define EXTRA 1

typedef int FOO;

extern int shared;
int shared;

size_t steps;

int count_up(size_t limit) {
    int total;
    size_t count;

    total = 0;
    for (count = 0; count < limit; count = count + 1) {
        total = total + 1;
    }

    return(total);
}

int main() {
    extern int shared;
    FOO width;

    width = START;
    steps = width + EXTRA;
    shared = count_up(steps);

    printint(shared);
    printchar('!');
    printchar('\n');
    printint(width);
    return(0);
}
