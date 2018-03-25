#include "FAST.h"

/**
 * Function : fast_AllocDataBlockEntry(int key, int* physical_address)
 * 
 * @deatils     Find a physical page in Data Block.
 *              First, seperate key into block and offset.
 *              Second, translate logical block to physical block using block table.
 *              Return state of translated address.
 *              If state is ERASED or VALID, update state.
 *              ERASED -> VALID, VALID -> INVALID
 * 
 * Returns : 
 *  physical_address
 * 
 */

char fast_AllocDataBlockEntry(KEYT key, uint32_t* physical_address)
{
    //printf("Slow1 ");
    // Seperate key into block and offset
    uint32_t logical_block = BLOCK(key);
    uint32_t offset = OFFSET(key);
    //printf("Slow2 ");
    // Translate logical block to physical block
    int physical_block = BLOCK_TABLE(logical_block);
    //printf("Slow3 ");
    // Check state of ssd using tasnlated address
    *physical_address = ADDRESS(physical_block, offset);
    //printf("%d ", _NOB);
    //printf("%d ", NUMBER_OF_DATA_BLOCK);
    //printf("%ld ", key);
    //printf("%d %d ", key / PAGE_PER_BLOCK, logical_block);
    //printf("%d %d ", physical_block, offset);
    //printf("%d %d ", *physical_address, NUMBER_OF_PAGE);
    char state = GET_PAGE_STATE(*physical_address);
    //printf("Slow4 ");
    // Should Use Block Mapping Table
	if(state == VALID){
		SET_PAGE_STATE(*physical_address, INVALID);
	}
	else if(state == ERASED){
		SET_PAGE_STATE(*physical_address, VALID);
	}
    //printf("Slow5 \n");
    return state;
}