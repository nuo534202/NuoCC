struct pair {
    int first;
    char second;
};

typedef struct pair PAIR;

struct node {
    int value;
    struct node *next;
};

char buffer[sizeof(int) * 4];
int cell= sizeof(long);

int main() {
    int count= 0;

    print sizeof(char);
    print sizeof(int);
    print sizeof(long);
    print sizeof(char *);
    print sizeof(void *);

    print sizeof(struct pair);
    print sizeof(PAIR);
    print sizeof(struct node);
    print sizeof(struct node *);

    print sizeof(buffer);
    print sizeof(cell);
    print sizeof(buffer[0]);

    print sizeof(count= 5);
    print count;
    return(0);
}
