int add(int a, int b) {
    return(a + b);
}

int mul(int a, int b) {
    return(a * b);
}

int six(int a, int b, int c, int d, int e, int f) {
    return(a + b + c + d + e + f);
}

char second(char *text) {
    return(*(text + 1));
}

int sum(int n, int acc) {
    if (n == 0) {
        return(acc);
    }
    return(sum(n - 1, acc + n));
}

int main() {
    int x;

    print add(3, 4);
    print mul(add(1, 2), 4);
    print add(add(1, 2), add(3, 4));
    print six(1, 2, 3, 4, 5, 6);
    print 5 + six(1, 2, 3, 4, 5, 6);
    print six(1, 2, 3, 4, 5, six(1, 1, 1, 1, 1, 1));
    print second("Hello");
    print sum(5, 0);
    x= 9;
    print add(x, mul(2, 3));
    return(0);
}
