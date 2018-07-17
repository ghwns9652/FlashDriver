#ifndef _BM_INTERFACE_H_
#define _BM_INTERFACE_H_

#include "BM_common.h"
#include "BM_Block.h"
#include "BM_Heap.h"

/* Type of offset */
typedef uint32_t	offset_t;

int8_t		BM_CheckLastOffset(Block* blockArray, PBA_T PBA, uint32_t offset);

/* Function to validate/invalidate the Block */
int8_t		BM_ValidateBlock_PPA(Block* blockArray, PPA_T PPA);
int8_t		BM_ValidateBlock_PBA(Block* blockArray, PBA_T PBA);
int8_t		BM_InvalidateBlock_PPA(Block* blockArray, PPA_T PPA);
int8_t		BM_InvalidateBlock_PBA(Block* blockArray, PBA_T PBA);

/* Inline Functions for finding valid/invalid blocks */
static inline int8_t BM_IsValidBlock_PPA(Block* blockArray, PPA_T PPA)
{
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	return (blockArray[PBA].numValid == _PPB);
}

static inline int8_t BM_IsValidBlock_PBA(Block* blockArray, PBA_T PBA)
{
	return (blockArray[PBA].numValid == _PPB);
}

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

#endif // !_BM_INTERFACE_H_
