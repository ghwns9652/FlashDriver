#include "FAST.h"
/**
 * Function :
 * 
 * Description : 
 * 
 * Returns :
 *  No returns
 */

char fast_SwitchSWLogBlock(uint32_t log_block_number, request* const req)
{
	uint32_t data_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
	uint32_t log_block = sw_MappingTable->data->physical_block;

	FAST_Algorithm.li->trim_block(data_block, 0), false);
	SET_BLOCK_STATE(data_block, ERASED);	
	for(int i = 0; i < NUMBER_OF_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
	}

	SET_BLOCK_STATE(log_block, DATA_BLOCK);

    return (eNOERROR);
}
