#include <stddef.h>

/* A string too long for one line is written as strings one after another,
 * which are one string. */
char *message= "hello " "there, "
               "how " "are " "you?";

/* A name may be declared extern before its storage is given here, and the
 * other way round: the two leave one name behind. */
extern int shared;
int later;
int shared;
extern int later;

void nothing(void) { return; }

void maybe(int which) {
    if (which) { print 1; return; }

    print 2;
    return;
}

int main() {
    int i;
    int total;
    int *pointer;

    print message[0];
    print message[6];
    print message[12];
    print message[17];

    /* A ';' on its own is a statement which does nothing. */
    total= 0;
    for (i= 0; i < 4; i= i + 1)
        total= total + 1;

    print total;

    while (0) ;
    if (total) ; else print 99;
    for (i= 0; i < 2; i= i + 1) ;

    nothing();
    maybe(1);
    maybe(0);

    shared= 5;
    later= 6;
    print shared + later;

    /* '&&' and '||' ask each side whether it holds, so one side may be an
     * address where the other is a number, and the answer may be used
     * anywhere a value may. */
    pointer= &total;
    if (pointer && total > 0) { print 7; }
    if (pointer || pointer) { print 8; }
    print (pointer && total > 0) ? 9 : 10;
    return(0);
}
