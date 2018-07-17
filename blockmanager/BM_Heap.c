/* BM_HEAP */

#include "BM_Heap.h"

/*
 * Functions for numValid_map with Min-HEAP 
 */
// heap 함수들은 보드에서 돌릴 걸 생각하면 재귀적으로 짜면 안 된다. 수정 필요
// lsm tree에 있는 heap을 참고하면 좋을 것

 /* Make Min-Heap with the pointers of numValid_map by numValid in blockArray */
//int32_t BM_Minheap_numValid(Block* blockArray, nV_T** numValid_map)
int32_t BM_Minheap_numValid(Block* blockArray, Block** numValid_map)
{
	nV_T* temp_NV = (nV_T*)malloc(sizeof(nV_T) * _NOB);

	for (int i = 0; i < _NOB; ++i) {
		temp_NV[i] = BM_GETNUMVALID(numValid_map[i]);
	}

	BM__buildminheapNV(temp_NV, numValid_map); 

	free(temp_NV); 
	return 0;
}


/* Build min-heap by numValid */
//int32_t BM__buildminheapNV(nV_T* temp_NV, nV_T** numValid_map)
int32_t BM__buildminheapNV(nV_T* temp_NV, Block** numValid_map)
{
	int32_t i;
	for (i = _NOB / 2; i >= 0; --i) {
		BM__minheapifyNV(temp_NV, i, numValid_map);
	}
	return 0;
}

//int32_t BM__minheapifyNV(nV_T* temp_NV, int32_t i, nV_T** numValid_map)
int32_t BM__minheapifyNV(nV_T* temp_NV, int32_t i, Block** numValid_map)
{
	int32_t l = 2 * i + 1;
	int32_t r = 2 * i + 2;
	int32_t smallest;

	if (l <= _NOB - 1 && (unsigned)temp_NV[l] < (unsigned)temp_NV[i]) smallest = l;
	else smallest = i;

	if (r <= _NOB - 1 && (unsigned)temp_NV[r] < (unsigned)temp_NV[smallest])	smallest = r;
	if (smallest != i) {                      
		
		SWAP_NV(temp_NV[i], temp_NV[smallest]);
		SWAP_NV_PTR(numValid_map[i], numValid_map[smallest]); 
		
		BM__minheapifyNV(temp_NV, smallest, numValid_map);
	}
	return 0;
}


/*
 * Functions for numValid_map with MAX-HEAP 
 */

 /* Make Max-Heap with the pointers of numValid_map by numValid in blockArray */
// This function is not used..
//int32_t BM_Maxheap_numValid(Block* blockArray, nV_T** numValid_map)
int32_t BM_Maxheap_numValid(Block* blockArray, Block** numValid_map)
{
	nV_T* temp_NV = (nV_T*)malloc(sizeof(nV_T) * _NOB);

	for (int i = 0; i < _NOB; ++i) {
		//temp_NV[i] = blockArray[i].numValid;
		// Maybe need to add badblock check in here
		temp_NV[i] = BM_GETNUMVALID(numValid_map[i]);
	}

	BM__buildmaxheapNV(temp_NV, numValid_map); 

	free(temp_NV); 
	return 0;
}


/* Build max-heap by numValid */
//int32_t BM__buildmaxheapNV(nV_T* temp_NV, nV_T** numValid_map)
int32_t BM__buildmaxheapNV(nV_T* temp_NV, Block** numValid_map)
{
	int32_t i;
	for (i = _NOB / 2; i >= 0; --i) {
		BM__maxheapifyNV(temp_NV, i, numValid_map);
	}
	return 0;
}

//int32_t BM__maxheapifyNV(nV_T* temp_NV, int32_t i, nV_T** numValid_map)
int32_t BM__maxheapifyNV(nV_T* temp_NV, int32_t i, Block** numValid_map)
{
	int32_t l = 2 * i + 1;
	int32_t r = 2 * i + 2;
	int32_t largest;

	if (l <= _NOB - 1 && temp_NV[l] > temp_NV[i]) largest = l;
	else largest = i;

	if (r <= _NOB - 1 && temp_NV[r] > temp_NV[largest])	largest = r;
	if (largest != i) {                      
		
		SWAP_NV(temp_NV[i], temp_NV[largest]);
		SWAP_NV_PTR(numValid_map[i], numValid_map[largest]); 
		
		BM__maxheapifyNV(temp_NV, largest, numValid_map);
	}
	return 0;
}

