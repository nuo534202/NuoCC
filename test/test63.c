/*
 * A do loop runs its body once before its condition is asked at all, which
 * is what tells it apart from a while loop: a loop whose condition is false
 * from the first still runs the body once.
 *
 * A continue in the body goes on to the condition rather than back to the
 * body, since the condition is what decides whether there is a pass to
 * begin, and a break leaves the loop at once as it does anywhere else.
 */
int main() {
    int count;
    int total;
    int inner;

    /* The body runs once and the condition is asked afterwards. */
    count= 0;
    do {
        count= count + 1;
    } while (count < 5);
    print count;

    /* A condition which is false from the first still runs the body once. */
    total= 0;
    count= 10;
    do {
        total= total + 1;
    } while (count < 0);
    print total;

    /* A continue goes on to the condition, so the loop still counts and
     * still ends. */
    count= 0;
    total= 0;
    do {
        count= count + 1;
        if (count == 3)
            continue;
        total= total + count;
    } while (count < 5);
    print count;
    print total;

    /* A break leaves the loop, condition or no condition. */
    count= 0;
    do {
        count= count + 1;
        if (count == 4)
            break;
    } while (1);
    print count;

    /* A body which is one statement needs no braces. */
    count= 0;
    do
        count= count + 1;
    while (count < 3);
    print count;

    /* A loop within a loop, where a break leaves only the inner one. */
    count= 0;
    total= 0;
    do {
        inner= 0;
        do {
            inner= inner + 1;
            total= total + 1;
        } while (inner < 2);
        count= count + 1;
    } while (count < 3);
    print total;

    return(0);
}
