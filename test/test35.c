int main() {
    int x;
    int y;

    /* An if and an else each take one statement, so no braces are needed. */
    x = 5;
    if (x > 3)
        print 1;
    else
        print 2;

    if (x > 7)
        print 3;
    else
        print 4;

    /* So does the body of a while loop. */
    x = 0;
    while (x < 3)
        x = x + 1;
    print x;

    /* A for loop may write a list of expressions in its first and last
     * clause, and its body may be one statement. */
    y = 0;
    for (x = 0, y = 1; x < 3; x = x + 1, y = y + 2)
        print y;

    /* A clause may be left out altogether. */
    for (; x < 5; x = x + 1)
        print x;

    /* The statements of a switch arm need no braces either. */
    for (x = 0; x < 4; x = x + 1) {
        switch (x) {
            case 0: print 10; break;
            case 1: print 11;
                    print 12;
                    break;
            case 2: print 13;
            default: print 14;
        }
    }

    /* An else belongs to the if it stands nearest. */
    for (x = 0; x < 4; x = x + 1)
        if (x > 1)
            if (x > 2)
                print 100 + x;
            else
                print 200 + x;

    /* A block may stand wherever a statement may. */
    {
        print 300;
    }
    return(0);
}
