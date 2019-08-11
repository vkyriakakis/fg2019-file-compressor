#include "fg2019/codes.h"

#include <assert.h>
#include <stdlib.h>

#include "fg2019/const.h"
#include "fg2019/error.h"
#include "fg2019/minQueue.h"

// symbolT: Contains a symbol and the prefix code corresponding to it.
//          Needed to maintain the symbol -> (len,val) mapping after sorting.
typedef struct {
    unsigned int symbol;
    unsigned int codeVal;
    int codeLen;
} symbolT;

// initLeafNode(): Initalizes a leaf node of the Huffman tree.
static inline huffmanNodeT *initLeafNode(unsigned int symbol, size_t freq) {
    assert(freq >= 0);

    huffmanNodeT *leaf = malloc(sizeof(*leaf));
    if (!leaf) {
        reportError("malloc");
        return NULL;
    }

    leaf->symbol = symbol;
    leaf->left = leaf->right = NULL;
    leaf->freq = freq;

    return leaf;
}

// initInnerNode: Initializes an inner node of the Huffman tree
//               (corresponds to a composite symbol).
static inline huffmanNodeT *initInnerNode(huffmanNodeT *left,
                                          huffmanNodeT *right) {
    assert(left != NULL);
    assert(right != NULL);

    huffmanNodeT *node = malloc(sizeof(*node));
    if (!node) {
        reportError("malloc");
        return NULL;
    }

    node->left = left;
    node->right = right;
    node->freq = left->freq + right->freq;

    return node;
}

// freeHuffmanTree(): Frees the Huffman tree.
//                Assumes that root != NULL.
static void freeHuffmanTree(huffmanNodeT *root) {
    assert(root != NULL);

    if (root->left != NULL)
        freeHuffmanTree(root->left);
    if (root->right != NULL)
        freeHuffmanTree(root->right);

    free(root);
}

// initHuffmanTree(): Uses the textbook Huffman coding algorithm to initialize
//              the Huffman tree, using the frequencies of all the symbols.
static huffmanNodeT *initHuffmanTree(size_t freqs[SYM_NUM]) {
    minQueueT minQueue;
    huffmanNodeT **initialNodes;
    huffmanNodeT *node1, *node2, *newNode, *huffmanRoot;
    int nonZeroTotal = 0;
    int k, t;

    // Initialize the array of the initial nodes, excluding the symbols with
    //  freq = 0
    //  so that they will not waste heap operations.
    for (k = 0; k < SYM_NUM; k++)
        if (freqs[k] != 0)
            nonZeroTotal++;

    initialNodes = malloc(sizeof(*initialNodes) * nonZeroTotal);
    if (!initialNodes) {
        reportError("malloc");
        return NULL;
    }

    for (t = 0, k = 0; k < SYM_NUM; k++) {
        if (freqs[k]) {
            initialNodes[t] = initLeafNode(k, freqs[k]);
            if (!initialNodes[t]) {
                reportError("malloc");
                return NULL;
            }
            t++;
        }
    }

    // Use the array to initialize the min queue used in the algorithm,
    // no more than nonZeroTotal positions are needed in the min queue,
    // as in each step 2 symbols merge into 1, so #symbols =< nonZeroTotal.
    minQueueInit(&minQueue, initialNodes, nonZeroTotal);

    while (minQueue.nodeTotal > 1) {
        node1 = delMin(&minQueue);
        node2 = delMin(&minQueue);
        newNode = initInnerNode(node1, node2);
        if (!newNode) {
            reportError("malloc");
            return NULL;
        }
        minQueueIns(&minQueue, newNode);
    }

    // Remove the root from the min queue, before freeing the queue's array,
    //  as the array is used in the minQueue, not a copy.
    huffmanRoot = delMin(&minQueue);
    free(initialNodes);

    return huffmanRoot;
}

// compHuffmanLens(): Computes the Huffman code length correspoding to each
//  symbol by recursively traversing the symbol tree.
//  Assumptions:
//   > codeLens = 0 when the function is called outside itself.
//   > symbols[] is sorted in such a way that symbols[k].symbol == k, where k is
//   a
//  symbol.
static void compHuffmanLens(huffmanNodeT *huffmanNode, symbolT symbols[SYM_NUM],
                            int codeLen) {
    assert(huffmanNode != NULL);

    if (huffmanNode->left == NULL && huffmanNode->right == NULL) {
        symbols[huffmanNode->symbol].codeLen = codeLen;
        return;
    }

    if (huffmanNode->left != NULL)
        compHuffmanLens(huffmanNode->left, symbols, codeLen + 1);
    if (huffmanNode->right != NULL)
        compHuffmanLens(huffmanNode->right, symbols, codeLen + 1);
}

// lenComp(): For use in qsort(), compares 2 symbols by code length.
static int lenComp(const void *ptr1, const void *ptr2) {
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);

    symbolT sym1 = *(symbolT *) ptr1;
    symbolT sym2 = *(symbolT *) ptr2;

    return sym1.codeLen - sym2.codeLen;
}

// lenThenLexComp(): For use in qsort(), first compares 2 symbols by code
//  length, then
//  lexicographically (by their numeric values).
static int lenThenLexComp(const void *ptr1, const void *ptr2) {
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);

    symbolT sym1 = *(symbolT *) ptr1;
    symbolT sym2 = *(symbolT *) ptr2;
    int diff = sym1.codeLen - sym2.codeLen;

    if (diff != 0)
        return diff;

    return sym1.symbol - sym2.symbol;
}

// limitCodeLens(): Limits code lengths to MAX_CODELEN, using the heuristic
//  algorithm
//  detailed here
//  http://cbloomrants.blogspot.com/2010/07/07-03-10-length-limitted-huffman-codes.html,
//  in order for the decompression lookup table scheme to be used.
//  Assumptions:
//   > symbols[] is sorted by increasing codeLen.

static void limitCodeLens(symbolT symbols[SYM_NUM]) {
    // kraftSum: The sum of 1 / 2^codeLen that appears in Kraft's inequality.
    double kraftSum = 0;
    int k;

    // 1. In the following loops, symbols with codeLen = 0 are ignored,
    //   because they do not appear in the file to be compressed.
    //   2. Casting to double is used to compute the Kraft inequality sum
    //   without
    //   using pow() from math.h

    for (k = 0; k < SYM_NUM; k++) {
        if (symbols[k].codeLen) {
            if (symbols[k].codeLen > MAX_CODELEN)
                symbols[k].codeLen = MAX_CODELEN;

            kraftSum += 1 / (double) (1 << symbols[k].codeLen);
        }
    }

    for (k = SYM_NUM - 1; k >= 0; k--)
        while (symbols[k].codeLen && symbols[k].codeLen < MAX_CODELEN &&
               kraftSum > 1) {
            symbols[k].codeLen++;
            kraftSum -= 1 / (double) (1 << symbols[k].codeLen);
        }

    for (k = 0; k < SYM_NUM; k++)
        while (symbols[k].codeLen &&
               (kraftSum + 1 / (double) (1 << symbols[k].codeLen)) <= 1) {
            kraftSum += 1 / (double) (1 << symbols[k].codeLen);
            symbols[k].codeLen--;
        }
}

// computeCodeVals(): Compute prefix code values using the Canonical Huffman
//  code
//  generation algorithm.
//  Assumptions:
//   > symbols[] is sorted by increasing codeLen and then lexicographically
//   (this
//  is needed in order
//   to enforce the same ordering of symbols with equal lengths during
//  compression and decompression, so
//    that the same code values are produced).
static void computeCodeVals(symbolT symbols[SYM_NUM]) {
    int prevLen, prevVal = 0;
    int k;

    for (k = 0; k < SYM_NUM; k++)
        if (symbols[k].codeLen) {
            symbols[k].codeVal = 0;
            prevLen = symbols[k].codeLen;
            break;
        }

    for (k++; k < SYM_NUM; k++)
        if (symbols[k].codeLen) {
            prevVal = symbols[k].codeVal = (prevVal + 1)
                                           << (symbols[k].codeLen - prevLen);
            prevLen = symbols[k].codeLen;
        }
}

int initCompressionTable(compTableT *compTablePtr, size_t freqs[SYM_NUM]) {
    huffmanNodeT *huffmanRoot;
    symbolT symbols[SYM_NUM];
    int k;

    assert(compTablePtr != NULL);

    huffmanRoot = initHuffmanTree(freqs);
    if (!huffmanRoot)
        return -1;

    // Iterate over all of the symbols and init symbols[]
    for (k = 0; k < SYM_NUM; k++) {
        symbols[k].symbol = k;

        // Not computed yet, codeLen init to 0 in order to indicate
        // a symbol that does not appear in the file to be compressed.
        symbols[k].codeLen = 0;
        symbols[k].codeVal = 0;
    }

    compHuffmanLens(huffmanRoot, symbols, 0);

    // The tree is not needed anymore
    freeHuffmanTree(huffmanRoot);

    // Sort symbol array by increasing code length, needed by limitCodeLens()
    qsort(symbols, SYM_NUM, sizeof(symbols[0]), lenComp);

    limitCodeLens(symbols);

    qsort(symbols, SYM_NUM, sizeof(symbols[0]), lenThenLexComp);

    computeCodeVals(symbols);

    // Fill the compression table
    for (k = 0; k < SYM_NUM; k++) {
        // symbols[k].symbol is used as the index
        // and not k, because due to the above sorting
        // symbols[k].symbols != k in the general case.
        compTablePtr->lens[symbols[k].symbol] = symbols[k].codeLen;
        compTablePtr->vals[symbols[k].symbol] = symbols[k].codeVal;
    }

    return 0;
}

int initDecompressionTable(decompTableT *decompTablePtr, int codeLens[SYM_NUM]) {
    symbolT symbols[SYM_NUM];
    int k, j;

    // curLen: Length of the current code inserted to the table
    // curVal: Value of the current code inserted to the table (determines the
    // positions of the table used)
    // curSym: Symbol corresponded to the current code, inserted to the
    // appropriate table positions
    int curLen;
    int curSym;

    assert(decompTablePtr != NULL);

    for (k = 0; k < SYM_NUM; k++) {
        symbols[k].symbol = k;
        symbols[k].codeLen = codeLens[k];
        symbols[k].codeVal = 0; // Not known yet
    }

    //  Sort symbol array by length and then lexicographically to produce the
    //  same codes values
    //  that were used in compression.
    qsort(symbols, SYM_NUM, sizeof(symbols[0]), lenThenLexComp);

    // Compute code vals using the same algo as in compression
    computeCodeVals(symbols);

    // Fill the decompression table according to the algorithm
    // detailed here
    // https://github.com/IJzerbaard/shortarticles/blob/master/huffmantable.md
    for (k = 0; k < SYM_NUM; k++) {
        if (symbols[k].codeLen != 0) {
            int first, last; // first and last indexes to be filled in the table
            curSym = symbols[k].symbol;
            curLen = symbols[k].codeLen;

            first = symbols[k].codeVal << (MAX_CODELEN - curLen);
            last = ((symbols[k].codeVal + 1) << (MAX_CODELEN - curLen)) - 1;
            for (j = first; j <= last; j++) {
                decompTablePtr->codeLens[j] = curLen;
                decompTablePtr->symbols[j] = curSym;
            }
        }
    }

    return 0;
}