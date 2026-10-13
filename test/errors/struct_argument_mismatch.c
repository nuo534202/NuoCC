struct first { int x; }; struct second { int x; }; int read(struct first *ptr) { return(ptr->x); } int main() { struct second value; print read(&value); return(0); }
