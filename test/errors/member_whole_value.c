struct inner { int x; }; struct outer { struct inner child; }; int main() { struct outer value; value.child; return(0); }
