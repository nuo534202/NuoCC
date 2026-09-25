#include <stdio.h>

/*
 * The one function the language can call. The compiler emits a call to it
 * for the print statement, and generated programs are linked with this
 * file.
 */
void printint(long x) {
  printf("%ld\n", x);
}
