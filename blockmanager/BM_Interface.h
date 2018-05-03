#ifndef _BM_INTERFACE_H_
#define _BM_INTERFACE_H_

#include "BM_common.h"
#include "BM_Block.h"
#include "BM_Heap.h"


/* Function to validate the state of PPA corresponding PPA(argument) */
int32_t		BM_validate_ppa(Block* blockArray, PPA_T PPA);

/* Function to invalidate the state of PPA corresponding PPA(argument) */
int32_t		BM_invalidate_ppa(Block* blockArray, PPA_T PPA);


/* Function to check whether PPA(argument) is VALID or not */
int32_t		BM_is_valid_ppa(Block* blockArray, PPA_T PPA);

int32_t		BM_invalidate_all(Block* blockArray);
int32_t		BM_validate_all(Block* blockArray);

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
}

static inline int32_t BM_update_block_with_trim(Block* blockArray, PPA_T PPA)
{
	/* This function should be called when Trim */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);

	blockArray[PBA].PE_cycle++;
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
