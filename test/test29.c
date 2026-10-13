struct large {
    char padding[5000];
    int value;
    long wide;
};

struct large storage;

int equals(long actual, long expected) {
    if (actual == expected) { return(1); }
    return(0);
}

int main() {
    struct { char left; int value; char right; } local;
    struct large *ptr;
    int *value;
    ptr= &storage;
    storage.padding[4999]= 255;
    ptr->value= -1234;
    ptr->wide= 70000;
    value= &ptr->value;
    print *value;
    print equals(storage.wide, 70000);
    print storage.padding[4999];
    local.left= 'a';
    local.right= 'z';
    local.value= 900;
    local.left= 'b';
    print local.left;
    print local.value;
    print local.right;
    return(0);
}
