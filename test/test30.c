union value {
    char bytes[8];
    int  number;
    long wide;
};

union value global;
union value *pointer;

int read(union value *item) {
    return(item->bytes[1]);
}

int main() {
    union value local;
    union value nested;
    union value *link;
    int i;

    pointer= &global;
    local.bytes[0]= 1;
    local.bytes[1]= 2;
    local.bytes[2]= 3;
    local.bytes[3]= 4;
    print local.number;
    print read(&local);
    local.wide= 0;
    print local.number;
    print local.bytes[0];

    global.number= 258;
    print global.bytes[0];
    print global.bytes[1];
    pointer->wide= 1027;
    print pointer->number;
    print pointer->bytes[3];

    nested.number= 66051;
    link= &nested;
    print link->bytes[0];
    print link->bytes[1];
    print link->bytes[2];
    link->wide= 0;
    print nested.number;
    return(0);
}
