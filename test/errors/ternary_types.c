int main() {
    int flag;
    int *pointer;
    int value;

    /* The two values of a ternary expression have to be of one
     * type, and a number is not a pointer. */
    value= flag ? 1 : pointer;
    return(0);
}
