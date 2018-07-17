/* Block Manager Interface */
#include "BM_Interface.h"

#define METHOD 2	// Which method has better performance?

/* Interface Functions for editing blockArray */

/* Check Last offset */
int8_t		BM_CheckLastOffset(Block* blockArray, PBA_T PBA, uint32_t offset)
{
	if (blockArray[PBA].LastOffset < offset) {
		blockArray[PBA].LastOffset = offset;
		return 1;
	}
	else
		// Moving Block with target offset update
		return 0;
	/*
	else if (blockArray[PBA].LastOffset > offset) {
		// Moving Block with target offset update
		return 0;
	}
	*/
}


int8_t		BM_ValidateBlock_PPA(Block* blockArray, PPA_T PPA)
{
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	int numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB%8 > 0)	numItem++;

	for (int j=0; j<numItem; ++j)
		blockArray[PBA].ValidP[j] = BM_VALIDPAGE;
	blockArray[PBA].numValid = _PPB;

	return (eNOERROR);
}

int8_t		BM_ValidateBlock_PBA(Block* blockArray, PBA_T PBA)
{
	int numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB%8 > 0)	numItem++;

	for (int j=0; j<numItem; ++j)
		blockArray[PBA].ValidP[j] = BM_VALIDPAGE;
	blockArray[PBA].numValid = _PPB;

	return (eNOERROR);
}

int8_t		BM_InvalidateBlock_PPA(Block* blockArray, PPA_T PPA)
{
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	int numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB%8 > 0)	numItem++;

	//for (int j=0; j<numItem; ++j)
		//blockArray[PBA].ValidP[j] = BM_VALIDPAGE;
	memset(blockArray[PBA].ValidP, BM_INVALIDPAGE, numItem);
	blockArray[PBA].numValid = 0;

	return (eNOERROR);
}

int8_t		BM_InvalidateBlock_PBA(Block* blockArray, PBA_T PBA)
{
	int numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB%8 > 0)	numItem++;

	//for (int j=0; j<numItem; ++j)
		//blockArray[PBA].ValidP[j] = BM_VALIDPAGE;
	memset(blockArray[PBA].ValidP, BM_INVALIDPAGE, numItem);
	blockArray[PBA].numValid = 0;

	return (eNOERROR);
}

int32_t		BM_is_valid_ppa(Block* blockArray, PPA_T PPA) 
{
	/*
	 * Return whether parameter PPA is VALID or INVALID
	 * if valid -> return=1
	 * if invalid -> return=0
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	offset_t offset = PPA % _PPB;

	uint32_t index = offset / 8;
	offset = offset % 8;

	if (blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset))
		return 1; // is valid
	else
		return 0; // is invalid

#if 0
	printf("index: %d\n", index);
	printf("ValidP[%d]: %x\n", index, blockArray[PBA].ValidP[index]);
	printf("numValid: %d\n", blockArray[PBA].numValid);
	if (blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset))
		printf("is valid!!\n");
	else
		printf("is invalid!!!!!!!!\n");
#endif

	if (blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset))
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
	offset_t offset = PPA % _PPB;

	uint32_t index = offset / 8;
	offset = offset % 8;
	uint8_t off_num = (uint8_t)1<<offset;

	if (blockArray[PBA].ValidP[index] & (off_num)) // is valid?
		return 0;
	else { // is invalid. Do Validate.
		blockArray[PBA].ValidP[index] |= (off_num);
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
	offset_t offset = PPA % _PPB;

	uint32_t index = offset / 8;
	offset = offset % 8;
	uint8_t off_num = (uint8_t)1<<offset;

	if (blockArray[PBA].ValidP[index] & (off_num)) { // is valid?
		blockArray[PBA].ValidP[index] &= ~(off_num);
		blockArray[PBA].numValid--;
		return 1;
	}
	else  // is invalid.
		return 0;
}
int32_t		BM_invalidate_all(Block* blockArray)
{
	/* Invalidate All pages */
	int numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB%8 > 0)	numItem++;

	for (int i=0; i<_NOB; i++) {
		memset(blockArray[i].ValidP, BM_INVALIDPAGE, numItem);
		blockArray[i].numValid = 0;
	}
	return (eNOERROR);
}

int32_t		BM_validate_all(Block* blockArray)
{
	/* Validate All pages */
	int numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB%8 > 0)	numItem++;

	for (int i=0; i<_NOB; ++i) {
		for (int j=0; j<numItem; ++j)
			blockArray[i].ValidP[j] = BM_VALIDPAGE;
		blockArray[i].numValid = _PPB;
	}
	return (eNOERROR);
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
