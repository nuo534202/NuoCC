struct first { int x; }; struct second { int x; }; int read(struct first *ptr); int read(struct second *ptr) { return(ptr->x); } int main() { return(0); }
