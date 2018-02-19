#include "FAST.h"

/**
 * Function : fast_AllocDataBlockEntry(int key, int* physical_address)
 * 
 * Description : 
 *  Find a physical page.
 * 
 * Returns :
 *  physical_address
 * 
 */

char fast_AllocDataBlockEntry(KEYT key, uint32_t* physical_address)
{
    // 
    int logical_block = BLOCK(key);
    int physical_block = BLOCK_TABLE(logical_block);
    int offset = OFFSET(key);

    *physical_address = ADDRESS(physical_block, offset);
    char check = GET_PAGE_STATE(*physical_address);

    /* Should Use Block Mapping Table */
    logical_block = BLOCK(key);
    offset = OFFSET(key);
	physical_block = BLOCK_TABLE(logical_block);
	*physical_address = ADDRESS(physical_block, offset);
    check = GET_PAGE_STATE(*physical_address);
	if(check == VALID){
		SET_PAGE_STATE(*physical_address, INVALID);
	}
	else if(check == ERASED){
		SET_PAGE_STATE(*physical_address, VALID);
	}
    return check;
    /*
    if(check == ERASED){
        return (eNOERROR);
    }
    else if(check == VALID){
        // We should write to log block 
        SET_STATE(physical_address, INVALID);
        return (VALID);
    }
    else if(check == INVALID){
        return (INVALID);
    }

    return (UNEXPECTED);
    */
}
