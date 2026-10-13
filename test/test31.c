enum fred {
    apple = 1,
    banana,
    carrot,
    pear = 10,
    peach,
    mango,
    papaya
};

enum { red, green, blue };

typedef int FOO;
typedef char *STRING;
typedef FOO COUNT;

struct bar {
    int x;
    int y;
};

typedef struct bar BAR;

FOO width;
BAR box;

int main() {
    enum fred var1;
    typedef int LOCAL;
    enum { gold, silver };
    LOCAL mine;
    FOO local;
    COUNT shown;

    var1 = carrot + pear + mango;
    print var1;

    var1 = papaya - apple;
    print var1;

    print peach;
    print blue;

    mine = silver;
    print mine;

    local = 7;
    shown = 9;
    width = 4;
    print local + shown + width;

    box.x = 3;
    box.y = 5;
    print box.x * box.y;

    BAR second;
    second.x = 1;
    second.y = 2;
    print second.x + second.y;

    STRING text;
    text = "hi";
    print text[0];
    print text[1];
    return(0);
}
