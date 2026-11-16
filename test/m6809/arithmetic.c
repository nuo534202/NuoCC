/*
 * The code the 6809 target writes for these values is recorded beside this
 * source. A long is false only when both halves are zero, arithmetic on a
 * char keeps its low byte, and incrementing a long carries through both
 * halves.
 */
long wide;
long not_wide;
char small;

int main(void) {
    wide = 65536;
    if (wide) { print 1; }

    wide = 1;
    if (wide) { print 2; }

    wide = 0;
    if (wide) { print 3; } else { print 4; }

    small = 255;
    print ++small;
    print small;

    small = 0;
    print --small;
    print small;

    small = 200;
    print small * 3;
    print small << 1;

    char *small_address = &small;
    small = 255;
    print small_address[0]++;
    print small;
    small = 0;
    print small_address[0]--;
    print small;

    wide = 65536;
    not_wide = !wide;
    if (not_wide == 0) { print 5; }

    wide = 0x1034ffff;
    ++wide;
    if (wide == 0x10350000) { print 6; }
    --wide;
    if (wide == 0x1034ffff) { print 7; }

    return (0);
}
