#include "FAST.h"

/**
 * @brief		Switch operation for SW Log Block
 * @detail		Erase (Trim) Data Block that corresponds to SW Log Block \n
 * 				and change state of SW Log Block into Data Block.
 * @return	 	Error number
 */
char fast_SwitchSWLogBlock()
{
	SW_MappingInfo*		sw_MappingInfo;
	Block_MappingInfo*	block_MappingInfo;
	uint32_t 			logical_block;
	uint32_t			data_block;
	uint32_t			sw_log_block;

    sw_MappingInfo 		= tableInfo->sw_MappingTable->data;
    block_MappingInfo 	= tableInfo->block_MappingTable->data;

    logical_block 	= sw_MappingInfo->logical_block;
	data_block 		= BLOCK_TABLE(logical_block);
	sw_log_block 	= sw_MappingInfo->sw_log_block;

	FAST_Algorithm.li->trim_block(ADDRESS(data_block, 0), false);
	SET_BLOCK_STATE(data_block, ERASED);
	for(unsigned int i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
	}

	sw_MappingInfo->number_of_stored_sector = 0;
    sw_MappingInfo->sw_log_block = FIND_ERASED_BLOCK();
	SET_BLOCK_STATE(sw_log_block, DATA_BLOCK);
    SET_BLOCK_STATE(sw_MappingInfo->sw_log_block, SW_LOG_BLOCK);

    block_MappingInfo[logical_block].physical_block = sw_log_block;

    printf("Switch Merge\n");

    return (eNOERROR);
}
