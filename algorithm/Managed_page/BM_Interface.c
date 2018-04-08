/* Badblock Manager */
#include "BM_Interface.h"

#define METHOD 2	// Which method has better performance?

/* Interface Functions for editing blockArray */

int32_t		BM_validate_ppa(Block* blockArray, PPA_T PPA)
{
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;

	if (blockArray[PBA].ValidP[offset] == BM_VALIDPAGE)
		return (eNOERROR);
	else {
		blockArray[PBA].ValidP[offset] = BM_VALIDPAGE;
		blockArray[PBA].numValid++;
		return (eNOERROR);
	}
}


// Four basic interfaces

int32_t		BM_invalidate_ppa(Block* blockArray, uint32_t PPA)
{
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;

#if (METHOD == 1)
	// This METHOD is wrong
	blockArray[PBA].ValidP[offset] = BM_INVALIDPAGE;
	if (blockArray[PBA].ValidP[offset] != BM_INVALIDPAGE)
		blockArray[PBA].numValid--;
	return (eNOERROR);
#endif

#if (METHOD == 2)
	if (blockArray[PBA].ValidP[offset] == BM_INVALIDPAGE) {
		printf("Input PPA is already INVALID\n");
		return (eNOERROR);
	}
	else {
		blockArray[PBA].ValidP[offset] = BM_INVALIDPAGE;
		blockArray[PBA].numValid--;
		return (eNOERROR);
	}
#endif
}
int32_t		BM_is_invalid_ppa(Block* blockArray, uint32_t PPA) 
{
	/*
	 * Return whether parameter PPA is VALID or INVALID
	 * Current Implementation: using char array to express state
	 */

	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;


	/* if - else if should be switched if invalid page is more than valid page */
	if (blockArray[PBA].ValidP[offset] == BM_VALIDPAGE) {
		printf("Input PPA is VALID\n");
		return (0);
	}
	else if (blockArray[PBA].ValidP[offset] == BM_INVALIDPAGE) {
		printf("Input PPA is UNVALID\n");
		return (1);
	}
	else {
		printf("Error!\n");
		ERR(eBADVALIDPAGE_BM);
	}
}
uint32_t	BM_get_gc_victim(Block* blockArray, nV_T** numValid_map)
{
	/* Return PBA of victim block */
	/*
	 * Parameter: Array(Heap) of numValid pointer(numValid_map)
	 * numValid_map is a heap array, so find the root of the max-heap in numValid using Heap opeartion
	 */


	/* After this function, numValid_map will become Max-heap by numValid */
	BM_Minheap_numValid(blockArray, numValid_map);

	/* Make Block_pointer from numValid_pointer */
	void* ptr_min_nV_block = (void*)numValid_map[0] - sizeof(ValidP_T)*_NOP - sizeof(PBA_T);

	return *((PBA_T*)ptr_min_nV_block); // This means value of PBA of maxnV block
}

uint32_t	BM_get_minPE_block(Block* blockArray, PE_T** PE_map)
{
	/* Return PBA of minPE block whose PE_cycle is minimum */
	/*
	 * Parameter: Array(Heap) of PE_map pointer(PE_map)
	 * PE_map is a Min-heap array, so find the root of the min-heap in PE_cycle using Heap opeartion
	 */


	/* After this function, PE_map Min-heap by PE_cycle */
	BM_Minheap_PEcycle(blockArray, PE_map);

	/* Make Block_pointer from PE_cycle pointer */
	void* ptr_min_PE_block = PE_map[0] - sizeof(nV_T) - sizeof(ValidP_T)*_NOP - sizeof(PBA_T);

	return *((PBA_T*)ptr_min_PE_block); // This means value of PBA of minPE block

}


uint32_t	BM_get_worn_block(Block *blockArray, PE_T** PE_map)
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


int32_t BM_update_block_with_push(Block* blockArray, uint32_t PPA)
{
	/* This function should be called when Push */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % _PPB;

	blockArray[PBA].ValidP[offset] = BM_WRITTEN; /* Not Determined yet */ /* What is BM_WRITTEN? */

	blockArray[PBA].PE_cycle++;
}

int32_t BM_update_block_with_trim(Block* blockArray, uint32_t PPA)
{
	/* This function should be called when Trim */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);

	blockArray[PBA].PE_cycle++;
}

