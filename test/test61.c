#include <stddef.h>

/*
 * A short is an integer which takes two bytes, whatever the machine gives
 * an int, so a value of one is -32768 to 32767. Reading one gives the sign
 * of the value back, and a value which is wider than a short is only stored
 * in one behind a cast, which keeps the two bytes such a value can hold.
 */

short global_value = 1000;
short global_negative = -1000;
short global_table[3];

struct pair {
    char tag;
    short left;
    short right;
};

struct pair item;

/* A short is handed to a function and answered with as any other value. */
short twice(short number) {
    return (number * 2);
}

short add(short left, short right) {
    short total = left + right;

    return (total);
}

int main() {
    short local = 300;
    short negative = -7;
    short table[3];
    short *pointer = table;
    char letter = 'A';
    int wide = local;
    long longer = negative;

    /* The room a short takes is two bytes wherever it stands. */
    print sizeof(short);
    print sizeof(struct pair);
    print sizeof(global_table);

    /* A value of a short keeps its sign wherever it is kept. */
    print global_value;
    print global_negative;
    print local;
    print negative;
    print twice(negative);
    print add(local, negative);

    /* A short widens to an int or a long, and a wider value only becomes a
     * short behind a cast, which keeps the two bytes it can hold. */
    print wide;
    print (int)longer;
    print (short)70000;
    print (int)negative;
    print (int)(long)global_negative;

    /* A char widens to a short, and the negative of one is a short. */
    print letter;
    print (short)letter;
    print -letter;

    /* A pointer reaches the next short by the room one of them takes. */
    table[0] = 100;
    table[1] = -200;
    table[2] = 300;

    print *pointer;
    print *(pointer + 1);
    print pointer[2];
    *(pointer + 2) = -400;
    print table[2];

    /* Members of a struct and elements of a global array are shorts. */
    item.left = local;
    item.right = item.left + 1;
    print item.right;
    print (short)item.tag;

    global_table[0] = 11;
    global_table[2] = item.right;
    print global_table[0] + global_table[2];

    /* A short is compared as a number which has a sign. */
    if (negative < 0) { print 1; } else { print 2; }
    if (local > negative) { print 3; } else { print 4; }
    if (global_negative == -1000) { print 5; } else { print 6; }

    return (0);
}
