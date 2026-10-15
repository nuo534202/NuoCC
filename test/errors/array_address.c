int ary[3];

int main() {
    int *pointer;

    /* The name of an array is already the address of its first element,
     * and the address of the whole array is not a value the language
     * has. */
    pointer= &ary;
    return(0);
}
