#ifndef _BM_HEAP_H_
#define _BM_HEAP_H_

#include "BM_common.h"
#include "BM_Block.h"

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



#endif // !_BM_HEAP_H_
