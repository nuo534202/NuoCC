struct record {
    int number;
    char first;
    char second;
    long total;
};

struct node {
    int value;
    struct node *next;
};

struct container {
    char label;
    struct record data;
    int slots[3];
    char *text;
};

struct record global;
struct record *global_ptr;
struct node head;
struct node tail;
struct container box;
struct { int value; } anonymous;
int calls;

int equals(long actual, long expected) {
    if (actual == expected) { return(1); }
    return(0);
}

int read(struct record *item) {
    return(item->number);
}

struct record *get() {
    calls= calls + 1;
    return(&global);
}

int main() {
    struct record local;
    struct record *ptr;
    struct record **indirect;
    int *number;
    char *letter;
    int *slots;
    struct node *links[2];
    struct node **link;

    print global.number;
    print global.first;
    print equals(global.total, 0);
    global.number= 12;
    global.first= 'c';
    global.second= 'd';
    global.total= 4005;
    print global.number;
    print global.first;
    print global.second;
    print equals(global.total, 4005);
    print equals(global.number + global.first + global.total, 4116);

    global_ptr= &global;
    print equals(global_ptr->number + global_ptr->first + global_ptr->total, 4116);
    global_ptr->number= -13;
    print global.number;
    global_ptr->first= 'x';
    print global.second;
    number= &global_ptr->number;
    *number= 14;
    print *number;

    local.number= -20;
    local.first= 255;
    local.second= 'b';
    local.total= -4005;
    ptr= &local;
    print ptr->number;
    print ptr->first;
    print equals(ptr->total, -4005);
    letter= &local.first;
    *letter= 'a';
    print local.first;
    print local.second;
    print read(ptr);
    indirect= &ptr;
    (*indirect)->number= 21;
    print (*ptr).number;
    print read(&local);
    print ptr->number= global.number= 22;
    print local.number;

    head.value= 31;
    head.next= &tail;
    tail.value= 32;
    tail.next= &head;
    print head.next->value;
    head.next->next->value= 33;
    print head.value;
    print head.next->next->next->value;

    box.label= 'z';
    box.data.number= 41;
    box.data.total= 42;
    print box.data.number;
    print equals(box.data.total, 42);
    print box.label;
    box.slots[0]= 51;
    box.slots[1]= -52;
    box.slots[2]= 53;
    slots= box.slots;
    print *(slots + 1);
    number= &box.slots[2];
    *number= 54;
    print box.slots[2];
    box.text= "word";
    print box.text[1];
    print *box.text;
    box.data.number= 43;
    ptr= &box.data;
    print ptr->number;

    anonymous.value= 61;
    print anonymous.value;
    print get()->number;
    print calls;
    get()->number= 62;
    print calls;
    print get()->number + get()->number;
    print calls;
    print (ptr= &global)->number;
    (global.number);
    links[0]= &head;
    links[1]= &tail;
    print links[1]->value;
    link= links;
    print (*(1 + link))->value;
    return(0);
}
