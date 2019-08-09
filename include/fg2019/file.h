#ifndef FILE_GUARD

#define FILE_GUARD

#include "fg2019/codes.h"
#include "fg2019/const.h"
#include <stddef.h> // For size_t
#include <stdio.h>

/* The file format used for the compressed files is the
  following:

  > A header, which includes:
          1. The magic number "FG2019" in ASCII.
          2. The number of compressed data bytes (sizeof(size_t))
          3. The code lengths of all 256 possible byte values and of the EOF
  symbol,
           with values that do not appear in the original file being indicated
           with a length of 0, which are needed to generate the same codes
               as in compression, so that the data will be able to be decoded.

  > The compressed data bytes (their number is stored in the header),
    which always include the EOF symbol (of numeric value EOF_VAL) as
    the last encoded symbol to aid in decompression.
     Some bits of the last byte might be "padding" bits, used to fill
    the last byte when the compression data size is not divisible by 8
    (so the last bits do not fill a whole byte). Once the EOF symbol is
    decoded, no more bits are to be processed, so those "padding" bits are
  igneored.
*/

/* isEmpty(): Checks if the file is empty. */
int isEmpty(FILE *fptr);

/* countSyms(): Return the frequencies of all symbols (byte or EOF)
      in the file pointed to by src.
       Assumptions:
        > src != NULL
    */
int countSyms(FILE *src, size_t freqs[SYM_NUM]);

/* compress(): Compresses the file pointed to by src,
      and writes the compressed data to dest.
       Assumptions:
        > All arguements != NULL
    */
int compress(FILE *src, FILE *dest, compTableT *compTablePtr);

/* compress(): Decompresses the compressed file pointed to by src
      and writes the decompressed data to dest.
       Assumptions:
    > All arguements != NULL
    */
int decompress(FILE *src, FILE *dest, decompTableT decompTable, size_t compSize);

/* writeHeader(): Writes information necessary for decompression
 (the compression header) to the file pointed to by dest. */
int writeHeader(FILE *dest, compTableT *compTablePtr, size_t freqs[SYM_NUM]);

/* readHeader(): Reads the compression header, and stores the
  necessary information (code lengths and compressed data size). */
int readHeader(FILE *src, int codeLens[SYM_NUM], size_t *compSizePtr);

#endif
