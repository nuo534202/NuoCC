/*
 * The product is wider than the 32-bit value an integer literal node can
 * hold. On the 64-bit targets the expression must remain in the tree and be
 * worked out in the target's eight-byte long instead of being truncated
 * into a folded 32-bit literal.
 */
long product;

int main() {
    product = (long)100000 * 100000;
    print (int)(product / 100000);
    return(0);
}
