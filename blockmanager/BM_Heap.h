#ifndef _BM_HEAP_H_
#define _BM_HEAP_H_

#include "BM_common.h"
#include "BM_Block.h"

/*
 * Heap Functions for blockArray, numValid_map, PE_map
 */
int32_t BM_Maxheap_numValid(Block* blockArray, uint8_t* numValid_map[]);
int32_t BM__buildmaxheapNV(uint8_t* temp_NV, uint8_t* numValid_map[]);
int32_t BM__maxheapifyNV(uint8_t* temp_NV, int32_t i, uint8_t* numValid_map[]);

int32_t BM_SortPE(Block* blockArray, uint32_t* PE_map[]);
void BM__quicksort(uint32_t* temp_PE, int p, int r, uint32_t* PE_map[]); // If this function works, it would be maybe better to fix with 'quicksort_Optimized'



/*
* Heap Functions for ptrBlock
*/



void max_heapify_cnt_(Block* ptrBlock[], uint32_t size_, int32_t i);
void build_max_heap_cnt_(Block* ptrBlock[], uint32_t size);

/* --------------------------------- */



uint32_t heap_maximum_extract(int* list, int size);
uint32_t heap_maximum(int* list);
void build_max_heap(int* list, int size);
void max_heapify(int* list, int count, int i);
void heapSort(int* list, int count);

//void build_max_heap_cnt(Block* Block_list, uint32_t size);
//void max_heapify_cnt(Block* Block_list, uint32_t size_, int32_t i);

#define SWAP_PE(a, b)	\
	{ uint32_t temp; temp=a; a=b; b=temp; }
#define SWAP_PE_PTR(a, b)	\
	{ uint32_t* temp; temp=a; a=b; b=temp; }
#define SWAP_NV(a, b)	\
	{ uint8_t temp; temp=a; a=b; b=temp; }
#define SWAP_NV_PTR(a, b)	\
	{ uint8_t* temp; temp=a; a=b; b=temp; }

#endif // !_BM_HEAP_H_
