int total;

int add(int a, int b) { return(a + b); }

int main() {
    int fred;
    int jim;
    int k;

    /* Two locals of the same size, whose places in the frame have to be
     * worked out one after the other. */
    fred= 5;
    jim= 12;
    print fred + jim;

    /* A compound assignment whose right side holds a comparison and a
     * ternary, with the value of the target already held in a register. */
    k= 0;
    k += (fred > 4) ? fred : jim;
    print k;
    k += (jim > 4) ? fred : jim;
    print k;

    /* The same with calls, a logical operation and a nested ternary inside
     * the right side. */
    total= 0;
    total += add(1, 2) > 2 ? add(3, 4) : add(5, 6);
    print total;
    total += (total > 0) ? total * 2 : 1;
    print total;
    total += (fred > 0 && jim > 0) ? add(fred, jim) : 0;
    print total;
    total += (total > 100) ? (total > 1000 ? 1 : 2) : (total > 10 ? 3 : 4);
    print total;

    k= 1;
    k= k + ((fred > jim) ? 100 : 200);
    print k;
    print (fred > 4 ? 10 : 20) + (jim > 4 ? 30 : 40);
    return(0);
}
