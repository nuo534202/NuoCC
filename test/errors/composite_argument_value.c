struct left {
    int x;
};

union right {
    int y;
};

int read(struct left item) {
    return(item.x);
}

int main() {
    union right value;
    print read(value);
    return(0);
}
