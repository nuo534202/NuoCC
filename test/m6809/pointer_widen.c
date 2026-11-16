/*
 * An address is unsigned on this target. Widen the 16-bit address 0x8000
 * to a long and compare the complete value, so a sign-extended high half
 * is observable.
 */
int main() {
    int *pointer;
    long widened;

    pointer = (int *)0x8000;
    widened = (long)pointer;

    if (widened == 0x8000) { print 1; }

    return(0);
}
