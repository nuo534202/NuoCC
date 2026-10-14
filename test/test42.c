#include <stddef.h>

enum sizes { SMALL = 2 * 3, LARGE = 100 + 24 };
enum numbers { below = -1 - 2, after_below };

int text_length= 512 + 1;
int total= 10 + 6;
int double_total= (10 + 6) * 2;
char letter= 'a' + 2;
char wrapped= (char)('a' + 1024);
char *none= NULL;
char *also_none= (char *)0;
char *word= "hi";
long wide= 3;
int fred[2 + 3];
int sized[SMALL];
int table[]= { 1 + 1, 2 * 2, SMALL };
int box[2 + 1]= { 7 };

int main() {
    print total;
    print double_total;
    print text_length;
    print SMALL;
    print LARGE;
    print below;
    print after_below;
    print letter;
    print wrapped;
    if (wide == 3) { print 1; }
    print fred[4];
    print sized[5];
    print table[0];
    print table[1];
    print table[2];
    print box[0];
    print box[2];

    if (none) { print 1; } else { print 2; }
    if (also_none) { print 3; } else { print 4; }
    print word[0];
    print word[1];

    fred[2]= total;
    print fred[2];

    switch (total) {
        case 10 + 6: { print 5; break; }
        default: { print 6; }
    }
    return(0);
}
