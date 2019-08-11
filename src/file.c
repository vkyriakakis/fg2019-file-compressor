#include "fg2019/file.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "fg2019/codes.h"
#include "fg2019/const.h"
#include "fg2019/error.h"

// Magic number and length
#define MAGIC_NUM "FG2019"
#define MAGIC_LEN 6

// Size (in bytes) of the various buffers used
#define BUF_SIZE 1024

// Shift amount used to make the MAX_CODELEN LSBs of the integer
//  the MAX_CODELEN MSBs by left shifting, used in decoding.
#define LOOKUP_SHIFT (INT_SIZE - MAX_CODELEN)

//   The numerical value of the EOF symbol, used to simplify the
//  decoding process, as once an EOF symbol is decoded, no more
//  bits of the compressed file contain data of the original and
//  decoding can stop.
#define EOF_VAL 256

// mask(): Creates and AND mask for use in decoding,
// with the x least significant bits set to 1.
// Assumption:
//  > x =< INT_SIZE, a mask of bits set to 1, with
// more bits than an int would be of no use in the
// decoding algorithm as it is.
static inline unsigned int mask(int x) {
    if (x == INT_SIZE)
        return 0xFFFFFFFF;

    return (1 << x) - 1;
}

int isEmpty(FILE *fptr) {
    char c;

    // If the end-of-file is reached upon trying to read one character,
    // then the file is empty.

    assert(fptr != NULL);

    c = fgetc(fptr);
    if (c == EOF) {
        if (feof(fptr))
            fprintf(stderr, "The file is empty!\n");
        else
            reportError("fgetc");

        return 1;
    }

    // Put the character back into the stream.
    c = ungetc(c, fptr);
    if (c == EOF) {
        reportError("ungetc");
        return 1;
    }

    return 0;
}

int countSyms(FILE *src, size_t freqs[SYM_NUM]) {
    unsigned char buffer[BUF_SIZE];
    size_t bytesRead;
    int k;

    assert(src != NULL);

    memset(freqs, 0, sizeof(freqs[0]) * SYM_NUM);

    //   Read the data in chunks of BUF_SIZE bytes until the end-of-file, to
    //  perform
    //  less read operations,
    //  and count the appearances of each symbol.
    do {
        bytesRead = fread(buffer, 1, BUF_SIZE, src);
        if (ferror(src)) {
            reportError("fread");
            return -1;
        }

        for (k = 0; k < bytesRead; k++)
            freqs[buffer[k]] += 1;

    } while (!feof(src));

    //   Add one appearance for the special EOF symbol (not to be confused with
    //  the
    //  actual end-of-file)
    //  that is used in decoding.
    freqs[EOF_VAL] = 1;

    return 0;
}

int compress(FILE *src, FILE *dest, compTableT *compTablePtr) {
    assert(src != NULL);
    assert(dest != NULL);
    assert(compTablePtr != NULL);

    //  readBuf[]: Buffer used to read BUF_SIZE from src at once, an
    //  optimization
    //        to limit fread() function calls (1 call for BUF_SIZE bytes is
    //  cheaper
    //        than BUF_SIZE calls to read 1 byte).
    //
    //   writeBuf[]: Used for similar reasons to readBuf[], only for
    //        fwrite() function calls. Must be initialized to 0,
    //        so that OR operations involving it give the wanted results.
    unsigned char readBuf[BUF_SIZE], writeBuf[BUF_SIZE] = {0};

    // bitsRemCode: The bits of the code corresponding to the current
    // byte of the read buffer being encoded, that need to be written to the
    // write buffer.
    int bitsRemCode;

    // bitsRemByte: The bits remaining in the byte of the write buffer
    //  that is currently used. Initialized to CHAR_BIT, as the first byte
    //  will initially have all of its CHAR_BITbits unused.
    char bitsRemByte = CHAR_BIT;

    // wPos: The position of the current byte in writeBuf.
    int wPos = 0;
    int k;
    size_t bytesRead, bytesWritten;
    int *codeLens = compTablePtr->lens;
    unsigned int *codeVals = compTablePtr->vals;

    // curVal: The current code value being written to the write buffer.
    unsigned int curVal;

    do {
        bytesRead = fread(readBuf, 1, BUF_SIZE, src);
        if (ferror(src)) {
            reportError("fread");
            return -1;
        }

        for (k = 0; k < bytesRead; k++) {
            bitsRemCode = codeLens[readBuf[k]];
            curVal = codeVals[readBuf[k]];

            //   While the code bits that haven't been written yet do
            //  not fit in the current byte of the write buffer,
            //  write as many bits as possible to fit in the byte, then
            //  move on to the next byte of the buffer, or in the
            //  case that the buffer is full, flush it and then begin from wPos
            //  =
            //  0.
            while (bitsRemCode > bitsRemByte) {
                writeBuf[wPos] |= curVal >> (bitsRemCode - bitsRemByte);
                wPos++;
                if (wPos == BUF_SIZE) {
                    bytesWritten = fwrite(writeBuf, 1, BUF_SIZE, dest);
                    if (bytesWritten < BUF_SIZE) {
                        reportError("fwrite");
                        return -1;
                    }

                    // Clear the write buffer, else OR operations will produce
                    // garbage
                    memset(writeBuf, 0, BUF_SIZE);
                    wPos = 0;
                }

                // bitsRemByte bits of the code were written to the buffer,
                // so bitsRemCode - bitsRemByte remain.
                bitsRemCode -= bitsRemByte;

                // A new byte will be used, so all of its CHAR_BIT bits are free
                bitsRemByte = CHAR_BIT;
            }

            // Write the reamining bits of the code to the current byte
            writeBuf[wPos] |= curVal << (bitsRemByte - bitsRemCode);
            bitsRemByte -= bitsRemCode;
        }
    } while (!feof(src));

    // Another loop to write the encoded EOF symbol to the file.
    curVal = codeVals[EOF_VAL];
    bitsRemCode = codeLens[EOF_VAL];

    while (bitsRemCode > bitsRemByte) {
        writeBuf[wPos] |= curVal >> (bitsRemCode - bitsRemByte);
        wPos++;
        if (wPos == BUF_SIZE) {
            bytesWritten = fwrite(writeBuf, 1, BUF_SIZE, dest);
            if (bytesWritten < BUF_SIZE) {
                reportError("fwrite");
                return -1;
            }

            memset(writeBuf, 0, BUF_SIZE);
            wPos = 0;
        }
        bitsRemCode -= bitsRemByte;
        bitsRemByte = CHAR_BIT;
    }
    writeBuf[wPos] |= curVal << (bitsRemByte - bitsRemCode);
    bitsRemByte -= bitsRemCode;

    //  If wPos > 0 after the above while loop ends,
    //  some compressed data is still in the
    //  write buffer. In that case a last write to the
    //  file should be performed.
    bytesWritten = fwrite(writeBuf, 1, wPos + 1, dest);
    if (bytesWritten < wPos + 1) {
        reportError("fwrite");
        return -1;
    }

    return 0;
}

int writeHeader(FILE *dest, compTableT *compTablePtr, size_t freqs[SYM_NUM]) {
    // compSize: Size of the compressed data in bytes.
    size_t compSize = 0;
    size_t written;
    // lenBuf: Buffer that stores the codeLens to be written.
    unsigned char lenBuf[SYM_NUM];

    assert(dest != NULL);
    assert(compTablePtr != NULL);

    written = fwrite(MAGIC_NUM, 1, MAGIC_LEN, dest);
    if (written < MAGIC_LEN) {
        reportError("fwrite");
        return -1;
    }

    // Compute compSize in bits.
    for (int k = 0; k < SYM_NUM; k++)
        compSize += freqs[k] * compTablePtr->lens[k];

    // Convert compSize to byte size by ceil.
    compSize = (compSize % CHAR_BIT == 0) ? (compSize / CHAR_BIT)
                                          : (compSize / CHAR_BIT + 1);

    //   Write size to file (for error written != 0 is checked,
    //   due to writing only 1 item of size sizeof(size_t)).
    written = fwrite(&compSize, sizeof(compSize), 1, dest);
    if (written == 0) {
        reportError("fwrite");
        return -1;
    }

    //   Write the code lengths for every symbol (EOF included) to
    //  the file (even the 0 lengths for the symbols that do not appear).
    //  There is no need to provide further information, the codeVals
    //  can be computed using the codeLens using the canonical Huffman algorithm
    //  in code.c.
    for (int k = 0; k < SYM_NUM; k++)
        lenBuf[k] = compTablePtr->lens[k];

    written = fwrite(lenBuf, 1, SYM_NUM, dest);
    if (written < SYM_NUM) {
        reportError("fwrite");
        return -1;
    }

    return 0;
}

int readHeader(FILE *src, int codeLens[SYM_NUM], size_t *compSizePtr) {
    // compSize: Size of the compressed data in bytes.
    size_t compSize;

    // readBuf: Buffer used for reading from the file, both the magic number,
    //   and the codelengths of the SYM_NUM symbols fit into it.
    char readBuf[SYM_NUM];

    assert(src != NULL);
    assert(compSizePtr != NULL);

    // Read MAGIC_LEN bytes, if there are not enough, the file does not adhere
    // to
    // the format.
    fread(readBuf, 1, MAGIC_LEN, src);
    if (feof(src)) {
        fprintf(stderr, "%s:%d: Malformed magic num error.\n", __FILE__,
                __LINE__);
        return -1;
    }
    else if (ferror(src)) {
        reportError("fread");
        return -1;
    }

    if (strncmp(readBuf, MAGIC_NUM, MAGIC_LEN) != 0) {
        fprintf(stderr, "Magic number missing!\n");
        return -1;
    }

    // Read compSize from file.
    fread(&compSize, sizeof(compSize), 1, src);
    if (feof(src)) { // Again if not enough bits, the header is malformed
        fprintf(stderr, "%s:%d: Malformed header error.\n", __FILE__, __LINE__);
        return -1;
    }
    else if (ferror(src)) {
        reportError("fread");
        return -1;
    }

    *compSizePtr = compSize;

    // Read all of the code lengths.
    fread(readBuf, 1, SYM_NUM, src);
    if (feof(src)) {
        fprintf(stderr, "%s:%d: Malformed header error.\n", __FILE__, __LINE__);
        return -1;
    }
    else if (ferror(src)) {
        reportError("fread");
        return -1;
    }

    for (int k = 0; k < SYM_NUM; k++)
        codeLens[k] = readBuf[k];

    return 0;
}

int decompress(FILE *src, FILE *dest, decompTableT decompTable,
               size_t compSize) {
    assert(src != NULL);
    assert(dest != NULL);

    // readBuf, writeBuf: Used for the same reason as in compress()
    unsigned char readBuf[BUF_SIZE], writeBuf[BUF_SIZE];

    // Bits remaining in the current read buffer byte
    char bitsRem = CHAR_BIT;

    // wPos: write buffer position, rPos: read buffer positions
    int wPos = 0, rPos;
    size_t bytesRead, bytesWritten;
    int readLoops = (compSize % BUF_SIZE == 0) ? (compSize / BUF_SIZE)
                                               : (compSize / BUF_SIZE + 1);

    // decIdx: A 32bit buffer, the MAX_CODELEN most significant bits of
    // which are used as the index of the decoding lookup table, by doing decIdx
    // >> LOOKUP_SHIFT.
    // Initialized to 0, in order for the first bit write using OR operations to
    //  work correctly.
    unsigned int decIdx = 0;

    // Bits needed until all 32bits of decIdx are filled.
    int bitsNeeded = INT_SIZE;

    //  totalBytes: The total number of bytes that have been read, used to
    //  detect
    //  if the compressed data is less than that indicated by compSize.
    size_t totalBytes = 0;

    for (int k = 0; k < readLoops; k++) {
        // (Re)fill read buffer
        bytesRead = fread(readBuf, 1, BUF_SIZE, src);
        if (ferror(src)) {
            reportError("fread");
            return -1;
        }
        // If the end-of-file (not to be confused with the EOF symbol) is
        //  reached,
        //  and the total number of bytes read is less than compSize, then some
        //  bytes are missing. Else, it is just the last loop of filling the
        //  read
        //  buffer.
        else if (feof(src) && (totalBytes + bytesRead < compSize)) {
            fprintf(stderr,
                    "%s:%d: Malformed file error, less data than promised.\n",
                    __FILE__, __LINE__);
            return -1;
        }
        totalBytes += bytesRead;

        // Reset read buffer position
        rPos = 0;

        //   The decoding table decIdx is formed using the bits read from the
        //  file,
        //  and should always contain sizeof(int)*CHAR_BIT bits. The number of
        //  bits
        //  is arbitrary, as long as it is greater than MAX_CODELEN (so that
        //  there won't be a need to find more bits during decoding). The
        //  requirement
        //  for the number of bits to remain constant is to simplify the
        //  algorithm a bit, with decIdx mimicking a bit stream.
        //   Initially, decIdx is empty (bitsNeeded == INT_SIZE),
        //  so it is filled with bits from the read buffer.
        //   When the x most significant bits of decIdx are used to
        //  decode a symbol (which is written to the write buffer), they are
        //  consumed (removed from decIdx through left shifting by x).
        //  Those x bits are then replenished by bits of the read buffer (as
        //  mention, decIdx always contains sizeof(int)*CHAR_BIT bits), and
        //  if the read buffer is out of bits, the while(1){} loop is
        //  exited, and the read buffer is refilled.
        while (1) {
            // The approach to writing bits to decIdx is similar to that used
            // in compress() for writing bits to writeBuf[wPos], but a mask is
            //  needed because decIdx is not CHAR_BIT bits long (unlike
            //  writeBuf[wPos]),
            //  so if bits other than the bitsNeeded LSBs are != 0, the value of
            //  decIdx will be wrong.
            while (bitsNeeded > bitsRem && rPos < bytesRead) {
                // Left shift promotes unsigned char to int
                decIdx |= ((readBuf[rPos] << (bitsNeeded - bitsRem)) &
                           mask(bitsNeeded));
                rPos++;
                bitsNeeded -= bitsRem;
                bitsRem = CHAR_BIT;
            }

            if (rPos == bytesRead)
                break;

            decIdx |=
              ((readBuf[rPos] >> (bitsRem - bitsNeeded)) & mask(bitsNeeded));
            bitsRem -= bitsNeeded;

            writeBuf[wPos] = decompTable.symbols[decIdx >> LOOKUP_SHIFT];
            wPos++;

            // If the write buffer is full, flush and reset position
            if (wPos == BUF_SIZE) {
                bytesWritten = fwrite(writeBuf, 1, BUF_SIZE, dest);
                if (bytesWritten < BUF_SIZE) {
                    reportError("fwrite");
                    return -1;
                }

                wPos = 0;
            }

            bitsNeeded = decompTable.codeLens[decIdx >> LOOKUP_SHIFT];
            decIdx <<= bitsNeeded;
        }
    }

    // After all of the compressed file's bits have been read,
    //  bits stored in decIdx cannot be replenished, so the bits
    //  remaining in decIdx are decoded into the symbols of t
    //  he original file, until the EOF symbol is found.
    //   Assuming the compressed file is not corrupted, the EOF
    //  symbol should always be found.

    // Check if EOF was found using its numeric value.
    while (decompTable.symbols[decIdx >> LOOKUP_SHIFT] != EOF_VAL) {
        writeBuf[wPos] = decompTable.symbols[decIdx >> LOOKUP_SHIFT];
        wPos++;

        if (wPos == BUF_SIZE) {
            bytesWritten = fwrite(writeBuf, 1, BUF_SIZE, dest);
            if (bytesWritten < BUF_SIZE) {
                reportError("fwrite");
                return -1;
            }

            wPos = 0;
        }

        // Consume the bits of decIdx needed for decoding.
        bitsNeeded = decompTable.codeLens[decIdx >> LOOKUP_SHIFT];
        decIdx <<= bitsNeeded;
    }

    // If wPos > 0 after the above while loop ends,
    //  some data is still in the
    //  write buffer. In that case a last write to the
    //  file should be performed.
    if (wPos > 0) {
        bytesWritten = fwrite(writeBuf, 1, wPos, dest);
        if (bytesWritten < wPos) {
            reportError("fwrite");
            return -1;
        }
    }

    return 0;
}