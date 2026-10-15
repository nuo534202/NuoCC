#include <stddef.h>

int total(int *values, int count) {
    int i;
    int sum;

    sum= 0;
    for (i= 0; i < count; i= i + 1)
        sum= sum + values[i];

    return(sum);
}

int main() {
    int ary[5];
    char text[4];
    long wide[2];
    int *pointers[3];
    char letter;
    int after[3];
    int big[100];
    int i;
    int x;

    /* A local array lives in the frame beside the other locals, so writing
     * one element does not disturb any of them. */
    letter= 'A';
    for (i= 0; i < 5; i= i + 1) ary[i]= i * 10;
    for (i= 0; i < 3; i= i + 1) after[i]= i + 1;
    text[0]= 104; text[1]= 105;
    wide[1]= 1000;
    pointers[1]= &after[2];

    print total(ary, 5);
    print ary[0] + ary[4];
    print text[0];
    print text[1];
    if (wide[1] == 1000) { print 1; }
    print *pointers[1];
    print letter;
    print after[0] + after[2];

    /* The room a local array takes is part of the frame, and the elements
     * are read and written as any other array's. */
    print sizeof(ary);
    print sizeof(text);
    print sizeof(pointers);

    for (i= 0; i < 100; i= i + 1) big[i]= i;
    print big[99];
    print sizeof(big);

    /* A deep expression beside a large array, so that the values set aside
     * for it are put in the same frame. */
    x= after[0] + (after[1] + (after[2] + (big[0] + (big[1] + (big[2] +
       (big[3] + (big[4] + big[5])))))));
    print x;
    return(0);
}
