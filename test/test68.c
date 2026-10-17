/*
 * The increment and the decrement take a place rather than only the name of
 * a variable: a place a pointer reaches, an element of an array, and a
 * member of a value are all places the new value can go back into, so
 * `list[1]++`, `point->number++` and `thing.number--` all change what stands
 * there. What the operator yields is the value the place held before the
 * change when it is written after it, and the value it holds afterwards
 * when it is written before it.
 *
 * Stepping a pointer on which stands in such a place moves the pointer on
 * by the room of what it points at, the way it does for a name: a member
 * which is a pointer to a struct is moved on by the room the struct takes.
 */
struct node {
    int number;
    struct node *link;
};

struct node nodes[3];
struct node first;

int main() {
    int list[4];
    struct node *p;

    /* An element of an array is a place. */
    list[1] = 5;
    print list[1]++;
    print list[1];
    print ++list[1];
    print list[1];
    print list[1]--;
    print --list[1];
    print list[1];

    /* A member of a value is a place. */
    first.number = 20;
    print first.number++;
    print first.number;
    print --first.number;
    print first.number;

    /* A member reached through a pointer is a place. */
    p = &first;
    p->number = 3;
    print p->number++;
    print p->number;
    print ++p->number;

    /* A member of an element of an array of values is a place, and so is a
     * place reached through a pointer. */
    nodes[2].number = 7;
    nodes[2].number--;
    print nodes[2].number;
    print (*p).number++;

    p = nodes;
    p->number = 1;
    print (*p).number++;
    print p->number;

    /* Stepping a pointer which stands in a member on, and back. */
    nodes[0].link = nodes + 1;
    nodes[0].link--;
    print nodes[0].link == nodes;
    nodes[0].link++;
    print nodes[0].link->number;
    print nodes[0].link == nodes + 1;

    return (0);
}
