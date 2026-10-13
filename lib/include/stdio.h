#ifndef NUOCC_STDIO_H
#define NUOCC_STDIO_H

#include <stddef.h>

/*
 * The two output functions the runtime provides. printint() writes the
 * decimal form of a value and printchar() writes the one character whose
 * code it is given.
 */
void printint(int value);
void printchar(int value);

#endif  /* NUOCC_STDIO_H */
