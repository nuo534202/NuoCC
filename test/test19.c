int increment(int value) {
    return(value + 1);
}

char keep(char value) {
    return(value);
}

int read_pointer(int *value) {
    return(*value);
}

int sum_to(int value) {
    if (value == 0) {
        return(0);
    }
    return(value + sum_to(value - 1));
}

int main() {
    int result;
    result = increment(41);
    print result;
    print increment(result);
    print keep(65);
    print read_pointer(&result);
    print sum_to(4);
    print increment(keep(65));
    return(0);
}
