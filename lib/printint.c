#include <stdio.h>

/*
 * The functions the language can call. The compiler emits calls to them
 * for the print statements, and generated programs are linked with this
 * file.
 */
void printint(long x) {
  printf("%ld\n", x);
}

void printchar(long x) {
  putc((char)(x & 0x7f), stdout);
}
