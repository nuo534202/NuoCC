/*
 * A character array may be given a string where it is declared, the way a
 * pointer to a char may: the characters of the string become the values of
 * the elements, one element each, and the zero byte the string ends with
 * becomes one element more when the array has room for it. An array with
 * room for the characters alone therefore leaves the zero byte out and
 * ends at its last element.
 *
 * The characters are values the compiler already knows, so a global array
 * has them put in its storage and a local one is given them where the
 * declaration stands, the same way a list of values is.
 */
char greeting[] = "hello";
char roomy[8] = "hi";
char whole[4] = "word";
char empty[] = "";
char joined[] = "one " "string";

int main() {
    int i;
    char local[] = "world";
    char narrow[4] = "ab";

    /* A string is printed a character at a time, up to its zero byte. */
    for (i = 0; greeting[i] != 0; i = i + 1) printchar(greeting[i]);
    printchar('\n');
    for (i = 0; local[i] != 0; i = i + 1) printchar(local[i]);
    printchar('\n');
    for (i = 0; joined[i] != 0; i = i + 1) printchar(joined[i]);
    printchar('\n');

    /* The size is the characters and the zero byte. */
    print sizeof(greeting);
    print sizeof(empty);
    print sizeof(local);

    /* Room for more than the string needs is kept, and the elements it was
     * not given a value for start out at zero. */
    print sizeof(roomy);
    print roomy[0];
    print roomy[2];
    print roomy[7];

    /* An array with room for the characters alone leaves the zero byte
     * out, so its last element is the last character of the string. */
    print sizeof(whole);
    print whole[3];
    print narrow[2];
    print narrow[3];

    return (0);
}
