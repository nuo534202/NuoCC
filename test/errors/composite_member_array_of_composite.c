struct point {
    int x;
    int y;
};

struct line {
    struct point ends[2];
};

int main() {
    struct line l = { { { 1, 2 }, { 3, 4 } } };
    return(0);
}
