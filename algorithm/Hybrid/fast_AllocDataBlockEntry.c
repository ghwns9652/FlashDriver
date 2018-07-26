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
    uint32_t    logical_block;
    uint32_t    offset;
    int         data_block;
    char        state;
    
    // Seperate key into block and offset
    logical_block = BLOCK(key);
    offset = OFFSET(key);

    // Translate logical block to physical block
    data_block = BLOCK_TABLE(logical_block);

    // Set physical_address and state of physical_address using data_block and offset
    *physical_address = ADDRESS(data_block, offset);
    state = GET_PAGE_STATE(*physical_address);
    
    // Set state of physical_address dependent to previous state of physical_address
	if (state == VALID) {
		SET_PAGE_STATE(*physical_address, INVALID);
	}
	else if (state == ERASED) {
		SET_PAGE_STATE(*physical_address, VALID);
	}

    return state;
}