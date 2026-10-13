struct pair {
    int  first;
    char second;
};

struct node {
    int value;
    struct node *next;
};

struct table {
    char label;
    int  rows[3];
};

struct outer {
    struct inner {
        int deep;
    } inside;
    int plain;
};

struct { int x; char c; } anon;
struct pair  item;
struct node  head;
struct table grid;
struct outer hull;

int main() {
    struct pair  local;
    struct node *link;
    int value;

    link= &head;
    value= 3 * 7;
    print value;
    print value + 1;
    return(0);
}
