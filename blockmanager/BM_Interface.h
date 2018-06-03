#ifndef _BM_INTERFACE_H_
#define _BM_INTERFACE_H_

#include "BM_common.h"
#include "BM_Block.h"
#include "BM_Heap.h"

/* Type of offset */
typedef uint32_t	offset_t;

/* Function to validate the state of PPA corresponding PPA(argument) */
int32_t		BM_validate_ppa(Block* blockArray, PPA_T PPA);

/* Function to invalidate the state of PPA corresponding PPA(argument) */
int32_t		BM_invalidate_ppa(Block* blockArray, PPA_T PPA);


/* Function to check whether PPA(argument) is VALID or not */
int32_t		BM_is_valid_ppa(Block* blockArray, PPA_T PPA);

int32_t		BM_invalidate_all(Block* blockArray);
int32_t		BM_validate_all(Block* blockArray);

static inline int8_t BM_getindex_256(offset_t* ptr_offset)
{
	int8_t index;
	if (*ptr_offset < 128) {
		if (*ptr_offset < 64)	index = 0;
		else					index = 1;
	}
	else {
		if (*ptr_offset < 192)	index = 2;
		else					index = 3;
	}
	*ptr_offset %= 64;
	//printf("index: %d, offset: %d\n", index, *ptr_offset);
	return index;
}
static inline int8_t BM_getindex_512(offset_t* ptr_offset)
{
	int8_t index;
	//printf("getindex, offset: %d\n", *ptr_offset);
	if (*ptr_offset < 256) {
		if (*ptr_offset < 128) {
			if (*ptr_offset < 64)	index = 0;
			else					index = 1;
		}
		else {
			if (*ptr_offset < 192)	index = 2;
			else					index = 3;
		}
	}
	else {
		// *ptr_offset -= 256;
		if (*ptr_offset < 384) {
			if (*ptr_offset < 320)	index = 4;
			else					index = 5;
		}
		else {
			if (*ptr_offset < 448)	index = 6;
			else					index = 7;
		}
	}
	*ptr_offset %= 64;
	//printf("getindex, index: %d\n", index);
	return index;
}

/*
 * Function returning PBA of GC victim block
 * Select GC vimtim block which has maximum numValid
 */
uint32_t	BM_get_gc_victim(Block* blockArray, Block** numValid_map);


/*
 * Function returning PBA of minPE block
 */
uint32_t	BM_get_minPE_block(Block* blockArray, Block** PE_map);


/* 
 * Function to wear-leveling 
 */
uint32_t	BM_get_worn_block(Block *blockArray, Block** PE_map);


/* Function with primitive */
//int32_t BM_update_block_with_gc(Block* blockArray, PPA_T PPA);

static inline int32_t BM_update_block_with_push(Block* blockArray, PPA_T PPA)
{
	/* This function should be called when Push */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	//uint8_t offset = PPA % _PPB;

	//blockArray[PBA].ValidP[offset] = BM_WRITTEN; /* Not Determined yet */ /* What is BM_WRITTEN? */

	blockArray[PBA].PE_cycle++;
	return 0;
}

static inline int32_t BM_update_block_with_trim(Block* blockArray, PPA_T PPA)
{
	/* This function should be called when Trim */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
#if 0
	blockArray[PBA].numValid = _PPB;

	uint32_t numItem = sizeof(ValidP_T) * (_PPB/8); // number of ValidP elements
	if (_PPB/8 > 0)	numItem++;

	for (int j=0; j<numItem; ++j)
		blockArray[PBA].ValidP[j] = BM_VALIDPAGE;
#endif

	blockArray[PBA].PE_cycle++;
	return 0;
}


// FTL -> Flash (Top -> Bottom)
// If FTL wants to access(Read, Write, Trim, ...) contents of FLASH, use this function.
static inline PBA_T BM_FTLtoFLASH(Block* blockArray, PBA_T FTL_PBA)
{
	/* FTLs see virtual PBAs to ignore PE_cycle/BAD problems */
	return (blockArray[FTL_PBA].v_PBA);
}

// FTL <- Flash (Top <- Bottom)
// If FTL wants to get contents of FLASH, use this function.
// (WARNIG) FTLs don't need to use this function.
static inline PBA_T BM_FLASHtoFTL(Block* blockArray, PBA_T v_PBA)
{
	/* If you want the PBA whose v_PBA is 4, then the PBA is blockArray[4].o_PBA. */
	return (blockArray[v_PBA].o_PBA);
}


#endif // !_BM_INTERFACE_H_
