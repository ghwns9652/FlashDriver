#include "FAST.h"
/**
 * Function: SW_Merge()
 * 
 * @brief       Merge Operation for Sequential Write (SW) Log Block (Partial Merge)
 * @details     First, copy valid pages in data block into SW log block.\n
 *              Then, change statement of data block into erased block \n
 *              and statement of SW log block into data block.
 * @return      Error code for function call
 */

char fast_MergeSWLogBlock(request *const req)
{
    SW_MappingInfo*     sw_MappingInfo;
    value_set*          value;
	uint32_t            src_address;
	uint32_t            dst_address;

    uint32_t            logical_block;
    uint32_t            data_block;
    uint32_t            sw_log_block;
    uint32_t            new_sw_log_block;
    
    uint32_t            i;

    sw_MappingInfo = tableInfo->sw_MappingTable->data;

    logical_block = sw_MappingInfo->logical_block;
    data_block = BLOCK_TABLE(sw_MappingInfo->logical_block);
	sw_log_block = sw_MappingInfo->sw_log_block;

	for(i = sw_MappingInfo->number_of_stored_sector; i < PAGE_PER_BLOCK; i++){
        src_address = ADDRESS(BLOCK_TABLE(logical_block), i);
        if(GET_PAGE_STATE(src_address) == VALID){
            value = fast_ReadPage(src_address, req, NULL, 1);
            dst_address = ADDRESS(sw_log_block, i);
            fast_WritePage(dst_address, req, value, 1);
        }
	}

	FAST_Algorithm.li->trim_block(ADDRESS(BLOCK_TABLE(logical_block), 0), false);
	for(i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
	}

    tableInfo->block_MappingTable->data[logical_block].physical_block = sw_log_block;

    new_sw_log_block = FIND_ERASED_BLOCK();
    sw_MappingInfo->sw_log_block = new_sw_log_block;
	sw_MappingInfo->number_of_stored_sector = 0;

	SET_BLOCK_STATE(sw_log_block, DATA_BLOCK);
	SET_BLOCK_STATE(data_block, ERASED);
    SET_BLOCK_STATE(new_sw_log_block, SW_LOG_BLOCK);

    printf("Partial Merge\n");
    return (eNOERROR);
}