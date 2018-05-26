#include "FAST.h"
/**
 * @brief
 * @detail
 * @return
 * 
 * 
 * Function :
 * 
 * Description : 
 * 
 * Returns :
 *  No returns
 */

char fast_SwitchSWLogBlock()
{
    SW_MappingInfo* sw_MappingInfo = tableInfo->sw_MappingTable->data;
    Block_MappingInfo* block_MappingInfo = tableInfo->block_MappingTable->data;

    uint32_t logical_block = sw_MappingInfo->logical_block;
	uint32_t data_block = BLOCK_TABLE(logical_block);
	uint32_t sw_log_block = sw_MappingInfo->sw_log_block;

	FAST_Algorithm.li->trim_block(ADDRESS(data_block, 0), false);
	SET_BLOCK_STATE(data_block, ERASED);

	for(unsigned int i = 0; i < NUMBER_OF_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
	}
	sw_MappingInfo->number_of_stored_sector = 0;
    sw_MappingInfo->sw_log_block = FIND_ERASED_BLOCK();
	SET_BLOCK_STATE(sw_log_block, DATA_BLOCK);
    SET_BLOCK_STATE(data_block, SW_LOG_BLOCK);

    block_MappingInfo[logical_block].physical_block = sw_log_block;

    printf("Switch Merge\n");

    return (eNOERROR);
}
