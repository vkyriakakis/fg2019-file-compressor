#include "fg2019/minQueue.h"

#include <assert.h>
#include <stdlib.h>

#include "fg2019/codes.h"

static inline void swap(huffmanNodeT **array, int x, int y) {
    assert(array != NULL);

    huffmanNodeT *temp = array[x];
    array[x] = array[y];
    array[y] = temp;
}

// siftDown(): Sinks a node down the heap until the min heap
//  property is satisfied, by swapping it with the child of
//  minimum frequency.
//   Assumptions:
//    > minQueuePtr != NULL
//    > 0 <= pos < nodeTotal
static inline void siftDown(minQueueT *minQueuePtr, int pos) {
    assert(minQueuePtr != NULL);
    assert(pos >= 0);

    int minChild = 2 * pos + 1;
    int nodeTotal = minQueuePtr->nodeTotal;
    huffmanNodeT **array = minQueuePtr->array;

    while (minChild < nodeTotal) {
        if (minChild + 1 < nodeTotal &&
            array[minChild + 1]->freq < array[minChild]->freq)
            minChild++;

        if (array[pos]->freq > array[minChild]->freq) {
            swap(array, pos, minChild);
            pos = minChild;
            minChild = 2 * pos + 1;
        }
        else
            break;
    }
}

void minQueueInit(minQueueT *minQueuePtr, huffmanNodeT **initialNodes,
                  int nodeTotal) {
    assert(minQueuePtr != NULL);
    assert(initialNodes != NULL);
    assert(nodeTotal > 0);

    minQueuePtr->array = initialNodes;
    minQueuePtr->nodeTotal = nodeTotal;

    for (int pos = nodeTotal / 2 - 1; pos >= 0; pos--)
        siftDown(minQueuePtr, pos);
}

void minQueueIns(minQueueT *minQueuePtr, huffmanNodeT *node) {
    assert(minQueuePtr != NULL);
    assert(node != NULL);

    int pos = minQueuePtr->nodeTotal++;
    huffmanNodeT **array = minQueuePtr->array;

    array[pos] = node;

    while (pos > 0 && array[(pos - 1) / 2]->freq > node->freq) {
        swap(array, pos, (pos - 1) / 2);
        pos = (pos - 1) / 2;
    }
}

huffmanNodeT *delMin(minQueueT *minQueuePtr) {
    assert(minQueuePtr != NULL);

    huffmanNodeT *min = minQueuePtr->array[0];

    minQueuePtr->array[0] = minQueuePtr->array[--minQueuePtr->nodeTotal];

    siftDown(minQueuePtr, 0);

    return min;
}
