/* An array is like a pointer to its first element, with a few things it
 * will not let itself be used for. */
int ary[4]= { 1, 2, 3, 4 };
char word[3]= { 104, 105, 0 };
int matrix[3]= { 7, 8, 9 };
int *ptr;
int **row;

int sum(int *values, int count) {
    int total;
    int i;

    total= 0;
    for (i= 0; i < count; i= i + 1)
        total= total + values[i];

    return(total);
}

int main() {
    int local[3];

    /* The name of an array is the address of its first element, so it may
     * be held in a pointer and read either way. */
    ptr= ary;
    print *ptr;
    print ptr[2];
    print *ary;
    print *(ary + 1);
    print (ary + 1)[0];
    print sum(ary, 4);
    print sum(ary + 1, 2);

    /* Writing through either of them writes one array. */
    ptr[2]= 30;
    print ary[2];

    /* A pointer to a pointer may be indexed twice. */
    ptr= ary;
    row= &ptr;
    print row[0][1];

    /* The address of an element may be taken, and read through. */
    ptr= &ary[3];
    print *ptr;

    /* An element may be the target of a compound assignment. */
    ary[1] += 10;
    print ary[1];

    /* An array given where a pointer is wanted, and the sizes of each. */
    print sum(matrix, 3);
    print sizeof(ary);
    print sizeof(ptr);

    /* A local array is like any other. */
    local[0]= 9;
    local[2]= 5;
    ptr= local;
    print ptr[0];
    print local[2];

    /* The address of an array is a value, so it holds unless it is
     * nothing, and one of its characters may be read. */
    if (ary) { print 1; }
    print word[1];
    return(0);
}
