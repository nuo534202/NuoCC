/* A static name belongs to the file it is written in, so this file's
 * counter and fred are its own, whatever another file calls them. */
static int counter= 0;
static int fred(void) { return(counter++); }

int second(void);

int main() {
    int i;

    for (i= 0; i < 3; i= i + 1)
        print fred();

    print second();
    print second();
    return(0);
}
