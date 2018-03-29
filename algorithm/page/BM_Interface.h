#ifndef _BM_INTERFACE_H_
#define _BM_INTERFACE_H_

#include "BM_common.h"
#include "BM_Block.h"
#include "BM_Heap.h"


/* Function to invalidate the state of PPA corresponding PPA(argument) */
int32_t		BM_invalidate_ppa(Block* blockArray, uint32_t PPA);


/* Function to check whether PPA(argument) is VALID or not */
int32_t		BM_is_invalid_ppa(Block* blockArray, uint32_t PPA);


/*
 * Function returning PBA of GC victim block
 * Select GC vimtim block which has maximum numValid
 */
uint32_t	BM_get_gc_victim(Block* blockArray, uint8_t* numValid_map[]);


/*
 * Function returning PBA of minPE block
 */
uint32_t	BM_get_minPE_block(Block* blockArray, uint8_t* PE_map[]);


/* 
 * Function to wear-leveling 
 */
uint32_t	BM_get_worn_block(Block *blockArray, uint32_t* PE_map[]);


/* Function with primitive */
int32_t BM_update_block_with_gc(Block* blockArray, uint32_t PPA);
inline int32_t BM_update_block_with_push(Block* blockArray, uint32_t PPA);
inline int32_t BM_update_block_with_trim(Block* blockArray, uint32_t PPA);


#endif // !_BM_INTERFACE_H_
