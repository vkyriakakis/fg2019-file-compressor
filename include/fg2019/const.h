#ifndef CONST_GUARD

#define CONST_GUARD

/* The total number of possible symbols,
     meaning all possible 256 byte values and the
     special EOF symbol, defined to make decompression easier. */
#define SYM_NUM 257

// Size of an integer in bits.
#define INT_SIZE sizeof(int) * 8

#endif
