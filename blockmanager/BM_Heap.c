/* BM_HEAP */

#include "BM_Heap.h"

/*
 * Functions for numValid_map with MAX-HEAP 
 */

 /* Make Max-Heap with the pointers of numValid_map by numValid in blockArray */
int32_t BM_Maxheap_numValid(Block* blockArray, uint8_t* numValid_map[])
{
	nV_T* temp_NV = (uint8_t*)malloc(sizeof(uint8_t) * _NOB);

	for (int i = 0; i < _NOB; ++i) {
		temp_NV[i] = blockArray[i].numValid;
	}

	BM__buildmaxheapNV(temp_NV, numValid_map); 

	free(temp_NV); 
}


/* Build max-heap by numValid */
int32_t BM__buildmaxheapNV(uint8_t* temp_NV, uint8_t* numValid_map[])
{
	int32_t i;
	for (i = _NOB / 2; i >= 0; --i) {
		BM__maxheapifyNV(temp_NV, i, numValid_map);
	}
}

int32_t BM__maxheapifyNV(uint8_t* temp_NV, int32_t i, uint8_t* numValid_map[])
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
}

/*
 * Functions for PE_cycle with MIN-HEAP 
 * !!! FOLLOWINGS ARE NOT VERIFIED !!!
 */

 /* Make Min-Heap with the pointers of PE_map by PE_cycle in blockArray */
int32_t BM_Minheap_PEcycle(Block* blockArray, uint8_t* PE_map[])
{
	PE_T* temp_PE = (PE_T*)malloc(sizeof(PE_T) * _NOB);

	for (int i = 0; i < _NOB; ++i) {
		temp_PE[i] = blockArray[i].PE_cycle;
	}

	BM__buildminheapPE(temp_PE, PE_map); 

	free(temp_PE); 
}


/* Build min-heap by PE_cycle */
int32_t BM__buildminheapPE(PE_T* temp_PE, uint8_t* PE_map[])
{
	int32_t i;
	for (i = _NOB / 2; i >= 0; --i) {
		BM__minheapifyPE(temp_PE, i, PE_map); // Is that OKAY?? I don't know about 'MINheapify'
	}
}

int32_t BM__minheapifyPE(PE_T* temp_PE, int32_t i, uint8_t* PE_map[])
{
	int32_t l = 2 * i + 1;
	int32_t r = 2 * i + 2;
	int32_t smallest;

	if (l <= _NOB - 1 && temp_PE[l] < temp_PE[i]) smallest = l;
	else smallest = i;

	if (r <= _NOB - 1 && temp_PE[r] < temp_PE[smallest])	smallest = r;
	if (smallest != i) {
		
		SWAP_PE(temp_PE[i], temp_PE[smallest]);
		SWAP_PE_PTR(PE_map[i], PE_map[smallest]); 
		
		BM__minheapifyPE(temp_PE, smallest, PE_map);
	}
}




/*
 * Functions for PE_map with SORT
 */

/* Sorting the pointers of PE_map by PE_cycle in blockArray */
int32_t BM_SortPE(Block* blockArray, uint32_t* PE_map[])
{
	uint32_t* temp_PE = (uint32_t*)malloc(sizeof(uint32_t) * _NOB);

	for (int i = 0; i < _NOB; ++i) {
		temp_PE[i] = blockArray[i].PE_cycle;
	}

	BM__quicksort(temp_PE, 0, _NOB - 1, PE_map); // Need Verification of this function


	free(temp_PE);
	return(eNOERROR);
}

void BM__quicksort(uint32_t* temp_PE, int p, int r, uint32_t* PE_map[]) 
{
	int q;
	int x;
	int i;
	int j;
	if (p < r) { // partitioning code
		x = temp_PE[r];
		i = p - 1;
		for (j = p; j < r; j++) {
			if (temp_PE[j] <= x) {
				i++; SWAP_PE(temp_PE[i], temp_PE[j]); 
				SWAP_PE_PTR(PE_map[i], PE_map[j]);
			}
		}
		SWAP_PE(temp_PE[i + 1], temp_PE[r]);	SWAP_PE_PTR(PE_map[i + 1], PE_map[r]);
		q = i + 1; // divide(partitioning end)
		BM__quicksort(temp_PE, p, q - 1, PE_map); // recursive call 
		BM__quicksort(temp_PE, q + 1, r, PE_map);
	}
}
