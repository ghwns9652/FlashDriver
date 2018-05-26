#include "FAST.h"

/**
 * @brief       Allocate an empty page in data block for set operation
 * @deatils     Find a physical page in Data Block.\n
 *              First, seperate key into block and offset.\n
 *              Second, translate logical block to physical block using block table.\n
 *              Return state of translated address.\n
 *              If state is ERASED or VALID, update state.\n
 *              ERASED -> VALID, VALID -> INVALID
 * @param       key                 (IN) given address
 * @param       physical_address    (OUT) address for data block address
 * @returns     State of the page corresponding to given key
 */
char fast_AllocDataBlockEntry(KEYT key, uint32_t* physical_address)
{
    // Seperate key into block and offset
    uint32_t logical_block = BLOCK(key);
    uint32_t offset = OFFSET(key);

    // Translate logical block to physical block
    int data_block = BLOCK_TABLE(logical_block);

    // Check state of ssd using tasnlated address
    *physical_address = ADDRESS(data_block, offset);
    char state = GET_PAGE_STATE(*physical_address);
    
    // Should Use Block Mapping Table
	if (state == VALID) {
		SET_PAGE_STATE(*physical_address, INVALID);
	}
	else if (state == ERASED) {
		SET_PAGE_STATE(*physical_address, VALID);
	}
    return state;
}