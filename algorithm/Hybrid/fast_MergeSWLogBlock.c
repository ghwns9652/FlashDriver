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

char fast_MergeSWLogBlock(uint32_t logical_block, request* req)
{
    SW_MappingTable* sw_MappingTable = tableInfo->sw_MappingTable;
    uint32_t data_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
	//char state;
	
	uint32_t src_address;
	uint32_t dst_address;

	value_set* value = req->value;

	for(unsigned int i = sw_MappingTable->data->number_of_stored_sector; i < PAGE_PER_BLOCK; i++){
        src_address = ADDRESS(BLOCK_TABLE(sw_MappingTable->data->logical_block), i);
        if(GET_PAGE_STATE(src_address) == VALID){
            fast_ReadPage(src_address, req, 1);

            dst_address = ADDRESS(sw_MappingTable->data->physical_block, i);
            fast_WritePage(dst_address, req, 1);
        }
	}

	FAST_Algorithm.li->trim_block(ADDRESS(sw_MappingTable->data->physical_block, 0), false);
	FAST_Algorithm.li->trim_block(ADDRESS(BLOCK_TABLE(sw_MappingTable->data->logical_block), 0), false);

	uint32_t dst_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
	for(unsigned int i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(dst_block, i), ERASED);
	}

	SET_BLOCK_STATE(sw_MappingTable->data->physical_block, DATA_BLOCK);
	SET_BLOCK_STATE(BLOCK_TABLE(sw_MappingTable->data->logical_block), ERASED);

    tableInfo->block_MappingTable->data[logical_block].physical_block = sw_MappingTable->data->physical_block;
    printf("Partial Merge ");
    return (eNOERROR);
}