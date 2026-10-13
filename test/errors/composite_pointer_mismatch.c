struct left {
    int x;
};

union right {
    int x;
};

int main() {
    struct left l;
    union right r;
    struct left *p;
    p= &r;
    return(0);
}
