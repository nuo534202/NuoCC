char *first;
char *second;
char **row;
char **other;
char *words[3];

int head(char **text) {
    return(**text);
}

int main() {
    first= "abc";
    second= "xyz";

    row= &first;
    printchar(**row);
    printchar(*(*row + 1));
    printchar('\n');

    *row= "hello";
    printchar(*first);
    printchar(**row);
    printchar('\n');

    other= row;
    printchar(**other);
    printchar('\n');

    row= row + 1;
    printchar(**row);
    printchar(head(row));
    printchar('\n');

    printchar(head(&first));
    printchar('\n');

    words[0]= "one";
    words[1]= "two";
    words[2]= "three";
    printchar(*words[0]);
    printchar(*words[2]);
    printchar(**words);
    printchar(**(words + 1));
    printchar(head(words + 1));
    printchar('\n');
    return(0);
}
