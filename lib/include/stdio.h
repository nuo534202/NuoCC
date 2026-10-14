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

/*
 * A pointer which points at nothing. It is a void pointer so that it may be
 * given to any pointer, and a cast is what turns the zero into one.
 */
#ifndef NULL
#define NULL (void *)0
#endif

#endif  /* NUOCC_STDIO_H */
