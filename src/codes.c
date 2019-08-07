#include <stdlib.h>
#include "const.h"
#include "minQueue.h"
#include "error.h"
#include "codes.h"


/* symbolT: Contains a symbol and the prefix code corresponding to it.
           Needed to maintain the symbol -> (len,val) mapping after sorting. */
typedef struct {
	unsigned int symbol;
	unsigned int codeVal;
	int codeLen;
} symbolT;



// initLeafNode(): Initalizes a leaf node of the Huffman tree.
huffmanNodeT *initLeafNode(unsigned int symbol, size_t freq) {
	huffmanNodeT *leaf = malloc(sizeof(huffmanNodeT));
	if (!leaf) {
		reportError("malloc", __FILE__, __LINE__);
		return NULL;
	}

	leaf->symbol = symbol;
	leaf->left = leaf->right = NULL;
	leaf->freq = freq;

	return leaf;
}

/* initInnerNode: Initializes an inner node of the Huffman tree 
                (corresponds to a composite symbol). */
huffmanNodeT *initInnerNode(huffmanNodeT *left, huffmanNodeT *right) {
	huffmanNodeT *node = malloc(sizeof(huffmanNodeT));
	if (!node) {
		reportError("malloc", __FILE__, __LINE__);
		return NULL;
	}

	node->left = left;
	node->right = right;
	node->freq = left->freq + right->freq;

	return node;
}

/* freeHuffmanTree(): Frees the Huffman tree.
                    Assumes that root != NULL. */
void freeHuffmanTree(huffmanNodeT *root) {
	if (root->left != NULL)
		freeHuffmanTree(root->left);
	if (root->right != NULL)
		freeHuffmanTree(root->right);

	free(root);
}

/* initHuffmanTree(): Uses the textbook Huffman coding algorithm to initialize
                    the Huffman tree, using the frequencies of all the symbols. */
huffmanNodeT *initHuffmanTree(size_t freqs[SYM_NUM]) {
	minQueueT minQueue;
	huffmanNodeT **initialNodes;
	huffmanNodeT *node1, *node2, *newNode, *huffmanRoot;
	int nonZeroTotal = 0;
	int k, t;


	/* Initialize the array of the initial nodes, excluding the symbols with freq = 0
	  so that they will not waste heap operations. */
	for (k = 0 ; k < SYM_NUM ; k++)
		if (freqs[k] != 0)
			nonZeroTotal++;

	initialNodes = malloc(sizeof(huffmanNodeT*)*nonZeroTotal);
	if (!initialNodes) {
		reportError("malloc", __FILE__, __LINE__);
		return NULL;
	}

	for (t = 0, k = 0 ; k < SYM_NUM ; k++) {
		if (freqs[k]) {
			initialNodes[t] = initLeafNode(k, freqs[k]);
			if (!initialNodes[t]) {
				reportError("malloc", __FILE__, __LINE__);
				return NULL;
			}
			t++;
		}
	}

	/* Use the array to initialize the min queue used in the algorithm,
     no more than nonZeroTotal positions are needed in the min queue,
     as in each step 2 symbols merge into 1, so #symbols =< nonZeroTotal. */
	minQueueInit(&minQueue, initialNodes, nonZeroTotal);

	while (minQueue.nodeTotal > 1) {
		node1 = delMin(&minQueue);
		node2 = delMin(&minQueue);
		newNode = initInnerNode(node1, node2);
		if (!newNode) {
			reportError("malloc", __FILE__, __LINE__);
			return NULL;
		}
		minQueueIns(&minQueue, newNode);
	}

	/* Remove the root from the min queue, before freeing the queue's array,
	 as the array is used in the minQueue, not a copy. */
	huffmanRoot = delMin(&minQueue);
	free(initialNodes);

	return huffmanRoot;
}


/* compHuffmanLens(): Computes the Huffman code length correspoding to each
  symbol by recursively traversing the symbol tree.
  Assumptions:
   > codeLens = 0 when the function is called outside itself.
   > symbols[] is sorted in such a way that symbols[k].symbol == k, where k is a symbol. */
void compHuffmanLens(huffmanNodeT *huffmanNode, symbolT symbols[SYM_NUM], int codeLen) {
	if (huffmanNode->left == NULL && huffmanNode->right == NULL) {
		symbols[huffmanNode->symbol].codeLen = codeLen;
		return;
	}

	if (huffmanNode->left != NULL)
		compHuffmanLens(huffmanNode->left, symbols, codeLen+1);
	if (huffmanNode->right != NULL)
		compHuffmanLens(huffmanNode->right, symbols, codeLen+1);
}


// lenComp(): For use in qsort(), compares 2 symbols by code length.
int lenComp(const void *ptr1, const void *ptr2) {
	symbolT sym1 = *((symbolT*)ptr1);
	symbolT sym2 = *((symbolT*)ptr2);

	return sym1.codeLen - sym2.codeLen;	
}


/* lenThenLexComp(): For use in qsort(), first compares 2 symbols by code length, then
  lexicographically (by their numeric values). */
int lenThenLexComp(const void *ptr1, const void *ptr2) {
	symbolT sym1 = *((symbolT*)ptr1);
	symbolT sym2 = *((symbolT*)ptr2);
	int diff = sym1.codeLen - sym2.codeLen;	

	if (diff != 0)
		return diff;

	return sym1.symbol - sym2.symbol;	
}


/* limitCodeLens(): Limits code lengths to MAX_CODELEN, using the heuristic algorithm
  detailed here http://cbloomrants.blogspot.com/2010/07/07-03-10-length-limitted-huffman-codes.html,
  in order for the decompression lookup table scheme to be used. 
  Assumptions:
   > symbols[] is sorted by increasing codeLen.
*/

void limitCodeLens(symbolT symbols[SYM_NUM]) {
	// kraftSum: The sum of 1 / 2^codeLen that appears in Kraft's inequality.
	double kraftSum = 0; 
	int k;

	/* 1. In the following loops, symbols with codeLen = 0 are ignored,
	   because they do not appear in the file to be compressed.
	   2. Casting to double is used to compute the Kraft inequality sum without
	   using pow() from math.h */

	for (k = 0 ; k < SYM_NUM ; k++) {
		if (symbols[k].codeLen) {
			if (symbols[k].codeLen > MAX_CODELEN)
				symbols[k].codeLen = MAX_CODELEN;

			kraftSum += 1 / (double)(1 << symbols[k].codeLen);
		}
	}

	for (k = SYM_NUM-1 ; k >= 0 ; k--) 
		while (symbols[k].codeLen && symbols[k].codeLen < MAX_CODELEN && kraftSum > 1 ) {
		    symbols[k].codeLen++;
		    kraftSum -= 1 / (double)(1 << symbols[k].codeLen);
		}

	for (k = 0 ; k < SYM_NUM ; k++) 
		while (symbols[k].codeLen && (kraftSum + 1 / (double)(1 << symbols[k].codeLen)) <= 1) {
		    kraftSum += 1 / (double)(1 << symbols[k].codeLen);
		    symbols[k].codeLen--;
		}
}


/* computeCodeVals(): Compute prefix code values using the Canonical Huffman code
  generation algorithm.
  Assumptions:
   > symbols[] is sorted by increasing codeLen and then lexicographically (this is needed in order
    to enforce the same ordering of symbols with equal lengths during compression and decompression, so
    that the same code values are produced).
*/
void computeCodeVals(symbolT symbols[SYM_NUM]) {
	int prevLen, prevVal = 0;
	int k;

	for (k = 0 ; k < SYM_NUM ; k++)
		if (symbols[k].codeLen) {
			symbols[k].codeVal = 0;
			prevLen = symbols[k].codeLen;
			break;
		}

	for (k++ ; k < SYM_NUM ; k++)
		if (symbols[k].codeLen) {
			prevVal = symbols[k].codeVal = (prevVal + 1) << (symbols[k].codeLen - prevLen);
			prevLen = symbols[k].codeLen;
		}
}


/* initCompressionTable(): Initialize the lookup table used in compression.
  Assumptions:
   > compTablePtr != NULL
*/

int initCompressionTable(compTableT *compTablePtr, size_t freqs[SYM_NUM]) {
	huffmanNodeT *huffmanRoot;
	symbolT symbols[SYM_NUM]; 
	int k;


	huffmanRoot = initHuffmanTree(freqs);
	if (!huffmanRoot)
		return -1;

	// Iterate over all of the symbols and init symbols[]
	for (k = 0 ; k < SYM_NUM ; k++) {
		symbols[k].symbol = k;

		/* Not computed yet, codeLen init to 0 in order to indicate 
		 a symbol that does not appear in the file to be compressed. */
		symbols[k].codeLen = 0; 
		symbols[k].codeVal = 0; 
	}

	compHuffmanLens(huffmanRoot, symbols, 0);

	// The tree is not needed anymore
	freeHuffmanTree(huffmanRoot);

	// Sort symbol array by increasing code length, needed by limitCodeLens()
	qsort(symbols, SYM_NUM, sizeof(symbolT), lenComp);

	limitCodeLens(symbols);

	qsort(symbols, SYM_NUM, sizeof(symbolT), lenThenLexComp);

	computeCodeVals(symbols);

	// Fill the compression table
	for (k = 0 ; k < SYM_NUM ; k++) {
		/* symbols[k].symbol is used as the index
		 and not k, because due to the above sorting
		 symbols[k].symbols != k in the general case. */
		compTablePtr->lens[symbols[k].symbol] = symbols[k].codeLen; 
		compTablePtr->vals[symbols[k].symbol] = symbols[k].codeVal;
	}

	return 0;
}



/* initDecompressionTable(): Initialize the lookup table used in decompression.
	  Assumptions:
	   > decompTablePtr != NULL
*/
int initDecompressionTable(decompTableT *decompTablePtr, int codeLens[SYM_NUM]) {
	symbolT symbols[SYM_NUM];
	int k, j;
	/*
     leftVal: The codeVal of the currently checked symbol left shifted, 
	so that the codeLen least significant bits become the most
	significant.

     leftIdx: The current value of index left shifted, 
	so that the MAX_CODELEN least significant bits become the most
	significant.

     mask: AND mask of the form 111110....000, used to determine if 
      leftVal is a prefix of leftIdx.
    */
	int leftVal, leftIdx, mask;
	
	// curLen: Length of the current symbol checked                     
	int curLen;

	for (k = 0 ; k < SYM_NUM ; k++) {
		symbols[k].symbol = k;
		symbols[k].codeLen = codeLens[k];
		symbols[k].codeVal = 0; // Not known yet
	}

	/* Sort symbol array by length and then lexicographically to produce the same codes values
	  that were used in compression. */
	qsort(symbols, SYM_NUM, sizeof(symbolT), lenThenLexComp);

	// Compute code vals using the same algo as in compression
	computeCodeVals(symbols);

	/* The table is filled as follows:

	   For every MAX_CODELEN-bit integer k, find the 
	  prefix code C that is a prefix of k (in a prefix code, a code cannot be
	  prefixed by another, so only one code can be a prefix of k).
	   To find the prefix, check all of the codes (len, val pairs) in the inner loop,
	  The index k is left shifted so that the MAX_CODELEN LSBs become the MSBs,
	  and the value of the code being checked is left shifted so that the codeLen LSBs
	  become the MSBs. If k is AND masked by a mask of the form 111111111000..0,
	                                                            \codeLen/ 
	  and masked(k) == the shifted value of the current code, C is a prefix of k.
	   Upon finding C, codeLens[k] = the length of C (in bits),
	  symbols[k] = the symbol encoded by C. 
	*/
	for (k = 0 ; k < DECOMP_SIZE ; k++)
		for (j = 0 ; j < SYM_NUM ; j++) {
			curLen = symbols[j].codeLen;

			if (curLen != 0) { // Ignore the symbols that do not appear in the original file
				leftIdx = k << (INT_SIZE - MAX_CODELEN);
				mask = ((1 << curLen) - 1) << (INT_SIZE - curLen);
				leftVal = symbols[j].codeVal << (INT_SIZE - curLen);
		
				if ((leftIdx & mask) == leftVal) {
					decompTablePtr->codeLens[k] = curLen;
					decompTablePtr->symbols[k] = symbols[j].symbol;
					break;
				}
			}
		}

	return 0;
}