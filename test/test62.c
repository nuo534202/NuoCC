/*
 * A function which takes a variable number of arguments is declared with
 * '...' at the end of its parameter list. Only the arguments it names are
 * checked against its parameters; the rest are given whatever type an
 * argument is given when nothing says otherwise, so a value narrower than
 * an int is widened to one before it is passed.
 *
 * printf is the function of the system which every machine this compiler
 * builds for has, and it is linked in with the runtime, so it is what a
 * program of this test calls.
 */
int printf(char *format, ...);

int main() {
    int value;
    short small;
    char letter;
    long big;

    value= 42;
    small= -7;
    letter= 'A';
    big= 123456789;

    printf("value %d, letter %c, small %d, big %ld\n",
           value, letter, small, big);

    /* A value which is already as wide as an int, and a pointer, travel
     * as they are. */
    printf("%d %s\n", 65, "and a string");
    return(0);
}
