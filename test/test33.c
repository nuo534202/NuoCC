int main() {
    int x;
    int y;

    /* A break leaves the loop it stands in. */
    x = 0;
    while (1) {
        if (x == 3) { break; }
        print x;
        x = x + 1;
    }

    /* A continue goes on to the next pass at once. */
    x = 0;
    while (x < 6) {
        x = x + 1;
        if (x == 3) { continue; }
        print x;
    }

    /* A continue in a for loop still runs the step written at its end. */
    for (x = 0; x < 5; x = x + 1) {
        if (x == 2) { continue; }
        print x;
    }

    /* A break in an inner loop leaves only that loop. */
    y = 0;
    for (x = 0; x < 3; x = x + 1) {
        while (1) {
            if (y == 2) { break; }
            print y;
            y = y + 1;
        }
        print x;
    }
    return(0);
}
