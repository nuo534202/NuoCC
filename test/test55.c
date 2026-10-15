/* A file may use the storage another file defines, by declaring the names
 * it needs as extern. */
extern int shared;
extern int values[3];

int main() {
    print shared;
    print values[0];
    print values[2];
    print shared + values[1];
    return(0);
}
