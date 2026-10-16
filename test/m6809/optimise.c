/*
 * The code the 6809 target writes for this program is recorded beside it,
 * in optimise.s, and the suite compares what the compiler writes with what
 * is recorded. That is how the pass this target has over the code it wrote
 * is tested: the peephole optimiser is not allowed to change what a program
 * prints, so the code it leaves is the only thing which says whether it
 * works.
 *
 * The program is written so that the code the backend writes, before the
 * pass has been over it, holds every sequence the pass is written for: a
 * value stored in the place of a register and read straight back, which is
 * what an expression worked out and held for the next one leaves; an
 * address stored and read straight back, which is what naming the member of
 * a struct leaves; and a jump to the label standing next to it, which is
 * what a return leaves.
 */
struct pair {
    int left;
    int right;
};

struct pair thing;

int twice(int number) {
    return (number * 2);
}

void main() {
    thing.left = 4;
    thing.right = twice(thing.left);
    print thing.right;
}
