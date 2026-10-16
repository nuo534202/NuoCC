#include <stddef.h>

/*
 * An array declared inside a function may be given the values it starts
 * with, and they are stored into its elements one after another where the
 * declaration stands. Each value is therefore worked out while the program
 * runs, so it may be anything the program can work out there and not only
 * a number written in the source.
 *
 * The size may be left out, in which case the array holds exactly as many
 * elements as it was given values, and an element left over is given
 * nothing, the way an element of a global array which was not given a
 * value starts out holding nothing.
 */
int main() {
    int size;
    int k;
    char *text;

    size= 4;

    /* A list of numbers written where the array is declared. */
    int list[5]= { 1, 2, 3, 4, 5 };

    for (k= 0; k < 5; k= k + 1)
        print list[k];

    /* The values are worked out where the declaration stands, so they may
     * name the variables declared before it. */
    int counted[]= { size, size * 2, size * 3 };

    for (k= 0; k < 3; k= k + 1)
        print counted[k];

    /* An element which was given no value holds nothing. */
    int short_list[4]= { 10, 20 };

    for (k= 0; k < 4; k= k + 1)
        print short_list[k];

    /* An array of chars, and an array of pointers. */
    char letters[3]= { 'a', 'b', 'c' };

    for (k= 0; k < 3; k= k + 1)
        printchar(letters[k]);
    printchar('\n');

    char *words[3]= { "one", "two", "three" };

    for (k= 0; k < 3; k= k + 1)
    {
        for (text= words[k]; *text != 0; text= text + 1)
            printchar(*text);
        printchar(' ');
    }
    printchar('\n');

    /* An element left over is given nothing, so a pointer left over points
     * at nothing. */
    char *only[3]= { "here" };

    if (only[1] == NULL)
        print 1;
    if (only[2] == NULL)
        print 2;

    return(0);
}
