/*
 * The code the 6809 target writes for this program is recorded beside it,
 * in optimise.s, and the suite compares what the compiler writes with what
 * is recorded. That is how the pass this target has over the code it wrote
 * is tested: the peephole optimiser is not allowed to change what a program
 * prints, so the code it leaves is the only thing which says whether it
 * works.
 *
 * The program makes the backend write a value into a register's place and
 * read it straight back before a conditional branch. The pass must leave
 * that load because it sets the flags the branch uses. It also writes a
 * jump to the label standing next to it, which the pass may remove.
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
    int flag;

    flag = 0;
    if (flag) { print 0; }

    thing.left = 4;
    thing.right = twice(thing.left);
    print thing.right;
}
