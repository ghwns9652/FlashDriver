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

int32_t BM_Minheap_PEcycle(Block* blockArray, uint8_t* PE_map[]);
int32_t BM__buildminheapPE(PE_T* temp_PE, uint8_t* PE_map[]);
int32_t BM__minheapifyPE(PE_T* temp_PE, int32_t i, uint8_t* PE_map[]);



/* Sorting algorithms foor blockArray and PE_map */
int32_t BM_SortPE(Block* blockArray, uint32_t* PE_map[]);
void BM__quicksort(uint32_t* temp_PE, int p, int r, uint32_t* PE_map[]); // If this function works, it would be maybe better to fix with 'quicksort_Optimized'



/* --------------------------------- */
/*
 * SWAP Macros
 */

#define SWAP_PE(a, b)	\
	{ PE_T temp; temp=a; a=b; b=temp; }
#define SWAP_PE_PTR(a, b)	\
	{ PE_T* temp; temp=a; a=b; b=temp; }
#define SWAP_NV(a, b)	\
	{ nV_T temp; temp=a; a=b; b=temp; }
#define SWAP_NV_PTR(a, b)	\
	{ nV_T* temp; temp=a; a=b; b=temp; }

#endif // !_BM_HEAP_H_
