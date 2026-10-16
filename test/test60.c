/* The arithmetic of the widest type the language has, whose working is
 * what a machine with no instruction for it has to have done for it. Every
 * value which is printed here fits the two bytes a machine may give an
 * int, so the program prints the same thing whatever width a machine gives
 * its types, while the values it works out along the way need all four
 * bytes of a long. */
long large = 100000;
long negative = -100000;
long values[3];

long add(long a, long b) { return(a + b); }
long scale(long a, long b) { return(a * b); }
long share(long a, long b) { return(a / b); }
long left(long a, long b) { return(a % b); }

int main() {
    long local;
    long product;
    int i;

    /* A long added to and taken from another, whose answer is small even
     * though the two values are not. */
    print (int)add(large, negative);
    print (int)(large + negative + 1);
    print (int)(large - 100000);
    print (int)(negative + 100000);

    /* One long multiplied by another: the product needs all four bytes,
     * and is taken apart again by dividing it, which only gives what was
     * multiplied in back if both halves of the working are right. */
    product = scale(large, 3000);
    print (int)share(product, large);
    print (int)left(product, large);
    print (int)share(product, 100000000);
    print (int)share(scale(large, 20000), 1000000000);
    print (int)share(scale(negative, 3000), large);

    /* The same for a division and a remainder, which a machine without an
     * instruction for them has to work out a bit at a time. */
    print (int)share(large, 300);
    print (int)left(large, 300);
    print (int)share(negative, 300);
    print (int)left(negative, 300);
    print (int)share(large, 100000);
    print (int)left(large, 100000);
    print (int)left(large, 32768);

    /* A long stands in a variable, in an array and in the frame of a
     * function, and is read and written as a whole wherever it is. */
    local = large;
    values[0]= large;
    values[1]= large + large;
    values[2]= large + large + large;
    print (int)share(local, 1000);
    print (int)share(values[0], 1000);
    print (int)share(values[1], 1000);
    print (int)share(values[2], 1000);
    print (int)share(values[2] - values[0], 1000);

    /* A long compares with another long, whichever of them is the
     * negative one. */
    if (large > negative) { print 1; }
    if (negative < large) { print 2; }
    if (large == 100000) { print 3; }
    if (large != negative) { print 4; }
    if (negative <= negative) { print 5; }
    if (large >= large) { print 6; }
    if (values[1] > values[0]) { print 7; }
    if (values[1] < values[2]) { print 8; }
    if (product == 300000000) { print 9; }

    /* A long which is asked about in a loop, where the value it holds has
     * to survive the work the loop does. */
    i= 0;
    while (large > 0) {
        large= large - 30000;
        i= i + 1;
    }
    print i;
    print (int)large;
    return(0);
}
