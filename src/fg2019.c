#include <stdio.h>
#include <string.h>
#include "codes.h"
#include "error.h"
#include "file.h"

int main(int argc, char *argv[]) {
	FILE *src, *dest;

	if (argc < 4) {
		if (!strcmp(argv[1], "-H")) {
			printf("To compress, run with: ./fg2019 -C <source-name> <compressed-name>.\n");
			printf("To decompress, run with: ./fg2019 -D <source-name> <decompressed-name>.\n");
			return 0;
		}
		fprintf(stderr, "Not enough arguements, run with -H for help.\n");
		return 1;
	}

	// Open data source, if compression was chosen it is the file to be compressed, else a compressed file
	src = fopen(argv[2], "rb");
	if (!src) {
		reportError("fopen", __FILE__, __LINE__);
		return 1;
	}
	
	// The resulting compressed or decompressed file
	dest = fopen(argv[3], "wb");
	if (!src) {
		reportError("fopen", __FILE__, __LINE__);
		return 1;
	}

	// If compression was chosen
	if (!strcmp(argv[1], "-C")) {
		// compTable: Lookup table used in compression
		compTableT compTable; 

		/* freqs: Array storing the number of appearances for each
		   symbol in the original file, indexed by the symbol */
		size_t freqs[SYM_NUM]; 

		// Check if the file is empty, and if it is, return
		if (isEmpty(src))
			return 1;

		if (countSyms(src, freqs) < 0)
			return 1;

		if (initCompressionTable(&compTable, freqs) < 0)
			return 1;

		// Set file pos to the beginning after counting
		if (fseek(src, 0, SEEK_SET) == -1)
			return 1;

		// Write the header containing information used for decompression
		if (writeHeader(dest, &compTable, freqs) < 0)
			return 1;

		// Compress and write the data to dest
		if (compress(src, dest, &compTable) < 0)
			return 1;
	}
	// If decompression was chosen
	else if (!strcmp(argv[1], "-D")) {
		// compSize: The size of the compressed file (excluding the header) in bytes
		size_t compSize;

		/* codeLens: Array containing the length of the prefix code used for each symbol,
		  indexed by the symbol. */
		int codeLens[SYM_NUM];

		// decompTable: Lookup table used in decompression
		decompTableT decompTable;

		if (readHeader(src, codeLens, &compSize) < 0)
			return 1;

		if (initDecompressionTable(&decompTable, codeLens) < 0)
			return 1;

		// Decompress the file and read the data to dest
		if (decompress(src, dest, decompTable, compSize) < 0)
			return 1;
	}
	else {
		fprintf(stderr, "This flag is not supported, run with <program-name> -H for help.\n");
		return 1;
	}

	fclose(src); 
	fclose(dest);

	return 0;
}
