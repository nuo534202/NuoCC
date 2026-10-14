#include <stddef.h>

typedef int FOO;

struct pair {
    int first;
    int second;
};

int wide= 65535;
char low= (char)65535;
char *none= (char *)0;
int *nothing= NULL;
struct pair *missing= (struct pair *)0;

struct pair item;

int main() {
    int a= 258;
    char *bytes= (char *)&a;
    int *back= (int *)bytes;
    char c= 'A';
    void *any= &a;
    int *same= (int *)any;
    char *local_none= NULL;
    int *local_nothing= (void *)0;
    FOO counted= 3;

    /* A cast to a narrower type keeps only the bits it holds. */
    print low;
    print (char)wide;
    print (char)300;
    print (int)c;
    print (FOO)counted;

    /* A pointer may be cast to another pointer, and read through. */
    print *bytes;
    print bytes[1];
    print *back;
    print *same;

    /* A pointer which points at nothing is false. */
    if (nothing) { print 1; } else { print 2; }
    if (none) { print 3; } else { print 4; }
    if (missing) { print 5; } else { print 6; }
    if (local_none) { print 7; } else { print 8; }
    if (local_nothing) { print 9; } else { print 10; }
    return(0);
}
