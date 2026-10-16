/*
 * The code this target writes for this program is recorded beside it, in
 * optimise.s, and the suite compares what the compiler writes with what is
 * recorded. That is how the pass this target has over the code it wrote is
 * tested: the peephole optimiser is not allowed to change what a program
 * prints, so the code it leaves is the only thing which says whether it
 * works.
 *
 * The program is written so that the code the backend writes, before the
 * pass has been over it, holds every sequence the pass is written for: a
 * value of eight bytes held in the place of a register and read straight
 * back, which is what an expression worked out and held for the next one
 * leaves; a char held and read the same way; and a branch to the label
 * standing next to it, which is what a return leaves.
 *
 * The same program is recorded for the x86-64 target, where the pass is
 * written in the instructions of that machine instead.
 */
char letter;
long number;

int twice(int value) {
    return (value * 2);
}

void main() {
    char mark;
    long held;

    number= 3;
    held= number + 1;
    number= held;
    mark= 'a';
    printchar(mark);
    letter= mark;
    print twice(letter);
    print (int)number;
}
