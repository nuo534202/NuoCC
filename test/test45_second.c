/* The other file of this program has a counter and a fred of its own,
 * which are static and so are not the ones here. */
static int counter= 0;
static int fred(void) { return(counter += 10); }

int second(void) {
    return(fred());
}
