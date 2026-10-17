/*
 * An array of composite values holds elements whose room comes from the
 * definition of the struct and not from the type of the value, so an
 * element is reached by counting that room along from the first one. The
 * same walk is written three ways below: with an index, by stepping a
 * pointer along, and with a pointer a function was given.
 */
struct point {
    int x;
    int y;
};

/* A composite value which holds composite values, so that reaching a
 * member of a member takes two offsets rather than one. */
struct edge {
    struct point from;
    struct point to;
};

/* An array declared at the top level starts out holding nothing, since the
 * room the data section is given is a room of zero. */
struct point grid[4];
struct edge path[2];

/* Walk an array the function was given, which reaches every element of it
 * with an index of its own. */
int span(struct point *points, int count) {
    int total;
    int k;

    total = 0;

    for (k = 0; k < count; k = k + 1)
        total = total + points[k].x + points[k].y;

    return (total);
}

int main() {
    /*
     * A local array of composite values cannot be given the values its
     * elements start with, since a composite value cannot be written as a
     * whole, so an element which was not stored into holds the nothing an
     * element of any other local array is left holding.
     */
    struct point local[3];
    struct point *p;
    int k;

    print local[0].x;
    print local[2].y;
    print grid[3].y;

    /* Every element is reached by counting the room the whole element
     * takes along from the first one. */
    for (k = 0; k < 4; k = k + 1) {
        grid[k].x = k;
        grid[k].y = 10 * k;
    }

    print grid[0].y;
    print grid[3].x;
    print grid[3].y;
    print span(grid, 4);

    /*
     * Stepping a pointer to a composite value on moves it on by the room
     * the whole value takes and not by the room one of the members takes,
     * so the address four steps along reaches the fifth value of the
     * array rather than the fifth int of it.
     */
    p = grid;
    print p->x;
    p++;
    print p->x;
    ++p;
    print p[1].x;
    p -= 1;
    print p[1].y;
    p += 2;
    print p->y;
    p--;
    print p->x;
    --p;
    print p->x;
    p = p + 1;
    print p->y;

    /* A member of a member is reached through the room of both. */
    path[1].to.x = 7;
    path[1].to.y = 8;
    print path[1].to.x;
    print path[1].to.y;
    p = &path[1].from;
    p->y = 9;
    print path[1].from.y;
    print (*p).y;

    /* The room a composite value takes is what a pointer to one steps by:
     * a struct of two ints takes twice the room an int does. */
    print sizeof(struct point) / sizeof(int);
    print sizeof(struct edge) / sizeof(int);

    return (0);
}
