enum colour { red, green, blue };

int main() {
    int x;
    int y;

    /* A switch chooses an arm, and a break leaves it. */
    y = 0;
    for (x = 0; x < 5; x = x + 1) {
        switch (x) {
            case 1: { y = 5; break; }
            case 2: { y = 7; break; }
            case 3: { y = 9; }
            default: { y = 100; }
        }
        print y;
    }

    /* An enum value names a case. */
    x = green;
    switch (x) {
        case red: { print 10; break; }
        case green: { print 20; break; }
        case blue: { print 30; break; }
    }

    /* A switch without a default does nothing when nothing matches. */
    x = 99;
    switch (x) {
        case 1: { print 1000; break; }
    }
    print 7;

    /* A continue in a switch still belongs to the loop around it. */
    for (x = 0; x < 6; x = x + 1) {
        switch (x) {
            case 2: { continue; }
            case 4: { break; }
            default: { print x; }
        }
        print x + 100;
    }
    return(0);
}
