struct pair {
    int first;
    int second;
} one, two;

int value;
char *text, letter;
int numbers[3], count;

char *greet() {
    return("hi");
}

int add_first(int a, int *b) {
    return(a + *b);
}

int main() {
    int a, *b, **c;
    int x, y[2], z;
    char *row[2];

    a = 4;
    b = &a;
    c = &b;
    print a;
    print *b;
    print **c;

    x = 1;
    y[0] = 2;
    y[1] = 3;
    z = 4;
    print x + y[0] + y[1] + z;

    value = 5;
    one.first = 6;
    two.second = 7;
    print value + one.first + two.second;

    numbers[0] = 8;
    count = 9;
    letter = 'A';
    text = greet();
    print numbers[0] + count + letter;
    print text[0];

    row[0] = "ab";
    row[1] = "cd";
    print row[0][1];
    print row[1][0];

    print add_first(10, &value);
    return(0);
}
