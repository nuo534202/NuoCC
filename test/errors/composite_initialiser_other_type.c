struct point {
    int x;
    int y;
};

struct pair {
    int a;
    int b;
};

int main() {
    struct point p = { 1, 2 };
    struct pair q = p;
    return(0);
}
