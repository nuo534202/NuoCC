int add(int a, int b);
int six(int a, int b, int c, int d, int e, int f);
char second(char *text);
int sum(int n, int acc);
int three();

int main() {
    int value;

    value= add(3, 4);
    print value;
    print six(1, 2, 3, 4, 5, 6);
    print second("Hello");
    print sum(5, 0);
    print three();
    print add(add(1, 2), add(3, 4));
    return(0);
}

int add(int a, int b) {
    return(a + b);
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

int three() {
    return(3);
}
