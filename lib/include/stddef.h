#ifndef NUOCC_STDDEF_H
#define NUOCC_STDDEF_H

/*
 * The width of a size in bytes. The language has only signed integers, so
 * this is the signed type which is as wide as an address.
 */
typedef long size_t;

/* How far apart two addresses are, measured in bytes. */
typedef long ptrdiff_t;

/*
 * A pointer which points at nothing. It is a void pointer so that it may be
 * given to any pointer, and a cast is what turns the zero into one.
 */
#ifndef NULL
#define NULL (void *)0
#endif

#endif  /* NUOCC_STDDEF_H */
