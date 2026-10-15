/* A case with nothing written after it falls into the case below it. */
int pick(int which) {
    int result;

    result= 0;
    switch (which) {
        case 1:
        case 2:
            result= 10;
            break;
        case 3:
        case 4:
        case 5:
            result= 20;
            break;
        default:
            result= 99;
    }
    return(result);
}

/* The name of an array stands for the address of its first element, so it
 * may be given wherever a pointer is wanted. */
char word[3]= { 104, 105, 0 };
int numbers[4]= { 1, 2, 3, 4 };

int first(char *text) { return(text[0]); }

int sum(int *values, int count) {
    int total;
    int i;

    total= 0;
    for (i= 0; i < count; i= i + 1)
        total= total + values[i];

    return(total);
}

int main() {
    int i;

    for (i= 0; i < 7; i= i + 1)
        print pick(i);

    print first(word);
    print sum(numbers, 4);
    print sum(numbers, 2);

    /* The two logical operators answer with zero or with one. */
    print 1 && 1;
    print 1 && 0;
    print 0 && 1;
    print 1 || 0;
    print 0 || 0;
    print 0 || 1;
    return(0);
}
