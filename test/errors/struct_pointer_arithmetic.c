struct pair {
    int first;
    char second;
};

struct pair item;

int main() {
    struct pair *link;
    link= &item;
    link= link + 1;
    return(0);
}
