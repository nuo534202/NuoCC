void no_result(void) {
    char value;
    value = 1;
}

int frame_size(void) {
    char first;
    char second;
    char third;
    first = 1;
    second = 2;
    third = 3;
    return(first == 1 && second == 2 && third == 3);
}

int narrow_widths(void) {
    char byte;
    short half;
    int word;
    long whole;
    byte = 255;
    half = -2;
    word = -3;
    whole = -4;
    return(byte == 255 && half == -2 && word == -3 && whole == -4);
}

int array_widths(void) {
    char bytes[2];
    short halves[2];
    int words[2];
    long wholes[2];
    bytes[0] = 1;
    halves[0] = -2;
    words[0] = -3;
    wholes[0] = -4;
    bytes[1] = 10;
    halves[1] = -20;
    words[1] = -30;
    wholes[1] = -40;
    if (halves[1] == -21)
        return(0);
    return(bytes[0] == 1 && halves[0] == -2 && words[0] == -3 &&
           wholes[0] == -4 &&
           bytes[1] == 10 && halves[1] == -20 && words[1] == -30 &&
           wholes[1] == -40);
}

char narrow_argument(char value, short adjustment) {
    if (adjustment != -100)
        return(0);
    return(value);
}

int main(void) {
    no_result();
    if (!frame_size())
        return(1);
    if (!narrow_widths())
        return(2);
    if (!array_widths())
        return(3);
    if ((char)263 != 7)
        return(4);
    if ((short)65436 != -100)
        return(5);
    if (narrow_argument((char)263, (short)65436) != 7)
        return(6);
    return(0);
}
