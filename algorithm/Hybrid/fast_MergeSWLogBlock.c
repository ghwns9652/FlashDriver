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

char fast_MergeSWLogBlock(uint32_t logical_block)
{
    SW_MappingTable* sw_MappingTable = tableInfo->sw_MappingTable;
    uint32_t data_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
    uint32_t new_block = data_block + 1;
	char state;
	
	uint32_t src_address;
	uint32_t dst_address;
	FAST_Parameters* params;
	value_set* value = req->value;
	algo_req* my_req;

	while(new_block != data_block){
		if(GET_BLOCK_STATE(new_block) == ERASED){
			break;
		}

		if(new_block == data_block){
			return (eUNEXPECTED);
		}
		new_block = (new_block + 1) / NUMBER_OF_BLOCK;
	}

	for(int i = sw_MappingTable->data->number_of_stored_sector; i < PAGE_PER_BLOCK; i++){
        src_address = ADDRESS(BLOCK_TABLE(sw_MappingTable->data->logical_block), i);
		dst_address = ADDRESS(new_block, i);
		params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
		params->parents = req;

		my_req = (algo_req*)malloc(sizeof(algo_req));
		my_req->end_req = FAST_EndRequest;
		my_req->params = (void*)params;

		FAST_Algorithm.li->pull_data(src_address, PAGESIZE, value, 0, my_req, 0);
		FAST_Algorithm.li->push_data(dst_address, PAGESIZE, value, 0, my_req, 0);

        free(params);
        free(my_req);
	}

	FAST_Algorithm.li->trim_block(ADDRESS(sw_MappingTable->data->physical_block, 0), false);
	FAST_Algorithm.li->trim_block(ADDRESS(BLOCK_TABLE(sw_MappingTable->data->logical_block), 0), false);

	uint32_t dst_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
	for(int i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(dst_block, i), ERASED);
	}

	SET_BLOCK_STATE(sw_MappingTable->data->physical_block, ERASED);
	SET_BLOCK_STATE(BLOCK_TABLE(sw_MappingTable->data->logical_block), ERASED);
	SET_BLOCK_STATE(new_block, DATA_BLOCK);

    return (eNOERROR);
}
