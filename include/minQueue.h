#ifndef QUEUE_GUARD
	
	#define QUEUE_GUARD

	#include "codes.h"

	// Min queue implementation used in Huffman coding (encoding.c).
         

	// minQueueT: Binary min-heap implemented using arrays 
	typedef struct {
		/* Array of pointers to Huffman tree nodes. */
		huffmanNodeT **array;

		/* Number of positions in the array that are used (due to how
                  a binary heap works they are the adjacent positions [0, nodeTotal-1] */
		int nodeTotal;
	} minQueueT;
	
	/* minQueueInit(): Initialize the minimum queue, using an array of Huffman tree nodes.
          Assumptions:
           > minQueuePtr is not a NULL pointer.
	   > nodeTotal > 0
        */
	void minQueueInit(minQueueT *minQueuePtr, huffmanNodeT **initialNodes, int nodeTotal);

	/* minQueueIns(): Insert a Huffman tree root into the queue.
          Assumptions:
           > minQueuePtr is not a NULL pointer.
        */
	void minQueueIns(minQueueT *minQueuePtr, huffmanNodeT *node);

	/* delMin(): Remove the root of minimum frequency from the queue and return it.
          Assumptions:
           > minQueuePtr is not a NULL pointer.
        */
	huffmanNodeT *delMin(minQueueT *minQueuePtr);

#endif
