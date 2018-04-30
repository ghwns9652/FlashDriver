/* Badblock Manager */
#include "BM_Interface.h"

#define METHOD 2	// Which method has better performance?

/* Interface Functions for editing blockArray */

int32_t		BM_is_valid_ppa(Block* blockArray, PPA_T PPA) 
{
	/*
	 * Return whether parameter PPA is VALID or INVALID
	 * if valid -> return=1
	 * if invalid -> return=0
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;
	int8_t index;

	if (offset < 128) {
		if (offset < 64)	index = 0;
		else				index = 1;
	}
	else {
		if (offset < 192)	index = 2;
		else				index = 3;
	}
	offset %= 64;

	if (blockArray[PBA].ValidP[index] & (1<<offset))
		return 1; // is valid
	else
		return 0; // is invalid
}

int32_t		BM_validate_ppa(Block* blockArray, PPA_T PPA)
{
	/*
	 * if valid -> do nothing, return=0
	 * if invalid -> Update ValidP and numValid, return=1
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;
	int8_t index;

	if (offset < 128) {
		if (offset < 64)	index = 0;
		else				index = 1;
	}
	else {
		if (offset < 192)	index = 2;
		else				index = 3;
	}
	offset %= 64;

	if (blockArray[PBA].ValidP[index] & (1<<offset)) // is valid?
		return 0;
	else { // is invalid.
		blockArray[PBA].ValidP[index] |= (1<<offset);
		blockArray[PBA].numValid++;
		return 1;
	}
}

int32_t		BM_invalidate_ppa(Block* blockArray, PPA_T PPA)
{
	/*
	 * if valid -> Update ValidP and numValid, return=1
	 * if invalid -> do nothing, return=0
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;
	int8_t index;

	if (offset < 128) {
		if (offset < 64)	index = 0;
		else				index = 1;
	}
	else {
		if (offset < 192)	index = 2;
		else				index = 3;
	}
	offset %= 64;
	if (blockArray[PBA].ValidP[index] & (1<<offset)) { // is valid?
		blockArray[PBA].ValidP[index] &= ~(1<<offset);
		blockArray[PBA].numValid--;
		return 1;
	}
	else  // is invalid.
		return 0;
}
uint32_t	BM_get_gc_victim(Block* blockArray, Block** numValid_map)
{
	/* Return PBA of victim block */
	/*
	 * Parameter: Array(Heap) of numValid pointer(numValid_map)
	 * numValid_map is a heap array, so find the root of the max-heap in numValid using Heap opeartion
	 */


	/* After this function, numValid_map will become Max-heap by numValid */
	BM_Minheap_numValid(blockArray, numValid_map);

	/* Make Block_pointer from numValid_pointer */
	//void* ptr_min_nV_block = (void*)numValid_map[0] - sizeof(ValidP_T)*4 - sizeof(PBA_T);
	//void* ptr_min_nV_block = (void*)numValid_map[0] - sizeof(ValidP_T)*4 - sizeof(PBA_T) - 4;
	Block* ptr_min_nV_block = numValid_map[0];
#if 0
	printf("sizeof(ValidP_T), sizeof(PBA_T): %d and %d\n", sizeof(ValidP_T), sizeof(PBA_T));
	printf("blockArray: %p\n", ((void*)&(blockArray[0].numValid)) - 32 - 4);
	printf("blockArray(real): %p\n", blockArray);
	printf("blockArray.PBA 주소: %p\n", &(blockArray[0].PBA));
	printf("blockArray.ValidP[0] 주소): %p\n", &(blockArray[0].ValidP[0]));
	printf("blockArray.ValidP[1] 주소): %p\n", &(blockArray[0].ValidP[1]));
	printf("blockArray.ValidP[2] 주소): %p\n", &(blockArray[0].ValidP[2]));
	printf("blockArray.ValidP[3] 주소): %p\n", &(blockArray[0].ValidP[3]));
	printf("blockArray.numValid 주소: %p\n", &(blockArray[0].numValid));
	printf("blockArray.PE_cycle 주소: %p\n", &(blockArray[0].PE_cycle));
	printf("blockArray.ptrNV_data 주소: %p\n", &(blockArray[0].ptrNV_data));
	printf("blockArray.ptrPE_data 주소: %p\n", &(blockArray[0].ptrPE_data));
	printf("blockArray.BAD 주소: %p\n", &(blockArray[0].BAD));
	printf("blockArray.v_PBA 주소: %p\n", &(blockArray[0].v_PBA));
	printf("blockArray.o_PBA 주소: %p\n", &(blockArray[0].o_PBA));
	void* ptr = (void*)numValid_map[0];
	printf("ptr: %p\n", ptr);
	printf("++ptr: %p\n", ++ptr);
	printf("--ptr: %p\n", --ptr);
	printf("ptr_min_nV_block: %p\n", ptr_min_nV_block);

	printf("numValid_map[0]: %p\n", numValid_map[0]);
	printf("blockArray[0]의 주소: %p\n", blockArray);
	printf("blockArray[0].PBA: %d\n", BM_GETPBA(blockArray));
	printf("blockArray[1].PBA: %d\n", BM_GETPBA(blockArray+1));
	printf("blockArray[3]의 주소: %p\n", blockArray+3);
	printf("blockArray[3].numValid의 주소: %p\n", &(blockArray[3].numValid));
	printf("After minheap, numValid_map[0]이 가리키는 PBA: %d\n", BM_GETPBA(ptr_min_nV_block));
#endif
	//return *((PBA_T*)ptr_min_nV_block->PBA); // This means value of PBA of maxnV block
	return BM_GETPBA(ptr_min_nV_block);
}

uint32_t	BM_get_minPE_block(Block* blockArray, Block** PE_map)
{
	/* Return PBA of minPE block whose PE_cycle is minimum */
	/*
	 * Parameter: Array(Heap) of PE_map pointer(PE_map)
	 * PE_map is a Min-heap array, so find the root of the min-heap in PE_cycle using Heap opeartion
	 */


	/* After this function, PE_map Min-heap by PE_cycle */
	BM_Minheap_PEcycle(blockArray, PE_map);

	/* Make Block_pointer from PE_cycle pointer */
	//void* ptr_min_PE_block = (void*)PE_map[0] - sizeof(nV_T) - sizeof(ValidP_T)*4 - sizeof(PBA_T);
	Block* ptr_min_PE_block = PE_map[0];

	//return *((PBA_T*)ptr_min_PE_block); // This means value of PBA of minPE block
	return BM_GETPBA(ptr_min_PE_block);

}


uint32_t	BM_get_worn_block(Block *blockArray, Block** PE_map)
{
	/* Function which sorts PE_map by PE_cycle with ascending order */
	/*@
	 * Parameter: Array of PE_cycle pointer(PE_map)
	 * (Warning) We need 'SWAP REAL DATA IN FLASH'. Current codes have no this step.
	 */

	/* Sort PE_map by PE_cycle */
	BM_SortPE(blockArray, PE_map);
	
	return (eNOERROR);
}

int32_t BM_update_block_with_gc(Block* blockArray, uint32_t PPA)
{
	/* This function should be called when GC */
	/*
	 * Parameter: PPA(or PBA?)
	 * Update status of corresponding block
	 * numValid = 0, ValidP[] = 0
	 */

	PBA_T PBA = BM_PPA_TO_PBA(PPA);

	blockArray[PBA].numValid = 0;
	memset(blockArray[PBA].ValidP, BM_VALIDPAGE, _NOP);

	return (eNOERROR);
}
