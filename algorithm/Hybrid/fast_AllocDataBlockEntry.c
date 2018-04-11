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
    // Seperate key into block and offset
    uint32_t logical_block = BLOCK(key);
    uint32_t offset = OFFSET(key);

    // Translate logical block to physical block
    int physical_block = BLOCK_TABLE(logical_block);
    //printf("lobical_block : %d physical_block : %d ", logical_block, physical_block);
    // Check state of ssd using tasnlated address
    *physical_address = ADDRESS(physical_block, offset);
    char state = GET_PAGE_STATE(*physical_address);
    
    // Should Use Block Mapping Table
	if(state == VALID){
		SET_PAGE_STATE(*physical_address, INVALID);
	}
	else if(state == ERASED){
		SET_PAGE_STATE(*physical_address, VALID);
	}
    return state;
}