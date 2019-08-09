#ifndef CODE_GUARD

#define CODE_GUARD

#include "fg2019/const.h"
#include <stddef.h> // For size_t

#define MAX_CODELEN 12                 // Max length of prefix codes
#define DECOMP_SIZE (1 << MAX_CODELEN) // Size of lookup array (2^max)

// prefixNodeT: Huffman tree node
struct huffmanNode {
    // freq: The frequency of the symbol corresponding to the node
    size_t freq;
    struct huffmanNode *left;
    struct huffmanNode *right;

    /* symbol: For a leaf node, the symbol correspoding to the node,
                  for an inner node it isn't used. */
    unsigned int symbol;
};
typedef struct huffmanNode huffmanNodeT;

/* compTableT: Lookup table used in compression,
        contains the prefix code and value for a given symbol.
        So, compression using the table is done by getting the
        (val, len) code pair for every byte of the original file. */
typedef struct {
    // Vals, lens are indexed by symbol numeric value
    unsigned int vals[SYM_NUM];
    int lens[SYM_NUM]; // in bits
} compTableT;

/* decompTableT: Lookup table used in decompression, as described in
  https://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art007 */

typedef struct {
    char codeLens[DECOMP_SIZE]; // in bits
    int symbols[DECOMP_SIZE];
} decompTableT;

/* initCompressionTable(): Initialize the lookup table used in compression.
  Input:
       > compTablePtr: Pointer to the compression table
       > freqs: Array containing the appearance frequency of all the symbols
      Output:
       >
       >
  Assumptions:
   > compTablePtr != NULL
*/
int initCompressionTable(compTableT *compTablePtr, size_t freqs[SYM_NUM]);

/* initDecompressionTable(): Initialize the lookup table used in decompression.
  Assumptions:
   > decompTablePtr != NULL
*/
int initDecompressionTable(decompTableT *decompTablePtr, int codeLens[SYM_NUM]);

#endif
