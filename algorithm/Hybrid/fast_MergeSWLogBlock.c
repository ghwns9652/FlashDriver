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

char fast_MergeSWLogBlock(uint32_t logical_block, request* const req)
{
    SW_MappingTable* sw_MappingTable = tableInfo->sw_MappingTable;
    uint32_t data_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
    uint32_t new_block = data_block + 1;
	//char state;
	
	uint32_t src_address;
	uint32_t dst_address;
	FAST_Parameters* params_1;
	FAST_Parameters* params_2;
	value_set* value = req->value;
	algo_req* my_req_1;
	algo_req* my_req_2;

    printf("%d %d\n", new_block, data_block);
	while(new_block != data_block){
        //printf("%d %d (Block Number)\n", new_block, data_block);
		if(GET_BLOCK_STATE(new_block) == ERASED){
			break;
		}
        //printf("%d", GET_BLOCK_STATE(new_block));
        /*
		if(new_block == data_block){
			return (eUNEXPECTED);
		}
        */
		new_block = (new_block + 1) % NUMBER_OF_BLOCK;
	}
    printf("%d %d %d %d \n", new_block, data_block, sw_MappingTable->data->logical_block, sw_MappingTable->data->physical_block);

	for(unsigned int i = sw_MappingTable->data->number_of_stored_sector; i < PAGE_PER_BLOCK; i++){
        src_address = ADDRESS(BLOCK_TABLE(sw_MappingTable->data->logical_block), i);

        if(GET_PAGE_STATE(src_address) == VALID){
            dst_address = ADDRESS(sw_MappingTable->data->physical_block, i);
            params_1 = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
            params_1->parents = req;
            params_1->test = -1;

            my_req_1 = (algo_req*)malloc(sizeof(algo_req));
            my_req_1->parents = NULL;
            my_req_1->end_req = FAST_EndRequest;
            //my_req_1->params = (void*)params_1;

            printf("%d %d (Address Test)\n", src_address, dst_address);
            FAST_Algorithm.li->pull_data(src_address, PAGESIZE, value, 0, my_req_1, 0);

            free(params_1);
            free(my_req_1);

            printf("%d %d (Address Test)\n", src_address, dst_address);

            dst_address = ADDRESS(sw_MappingTable->data->physical_block, i);
            params_2 = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
            params_2->parents = req;
            params_2->test = -1;

            my_req_2 = (algo_req*)malloc(sizeof(algo_req));
            my_req_2->parents = NULL;
            my_req_2->end_req = FAST_EndRequest;
            //my_req_2->params = (void*)params_2;

            printf("%d %d (Address Test)\n", src_address, dst_address);
            FAST_Algorithm.li->push_data(dst_address, PAGESIZE, value, 0, my_req_2, 0);

            printf("%d %d (Address Test)\n", src_address, dst_address);

            free(params_2);
            printf("%d %d (Address Test)\n", src_address, dst_address);
            free(my_req_2);
        }
	}

	FAST_Algorithm.li->trim_block(ADDRESS(sw_MappingTable->data->physical_block, 0), false);
	FAST_Algorithm.li->trim_block(ADDRESS(BLOCK_TABLE(sw_MappingTable->data->logical_block), 0), false);

	uint32_t dst_block = BLOCK_TABLE(sw_MappingTable->data->logical_block);
	for(unsigned int i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(dst_block, i), ERASED);
	}

	SET_BLOCK_STATE(sw_MappingTable->data->physical_block, ERASED);
	SET_BLOCK_STATE(BLOCK_TABLE(sw_MappingTable->data->logical_block), ERASED);
	SET_BLOCK_STATE(new_block, DATA_BLOCK);

    printf("Partial Merge ");
    return (eNOERROR);
}

/*
         for (i = 0; i < offset; ++i) {
            algo_req *temp_req=(algo_req*)malloc(sizeof(algo_req));
            int8_t* temp_block = (int8_t*)malloc(PAGESIZE);

            temp_req->parents = NULL;
            temp_req->end_req = block_algo_end_req;

            __block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_block, 0, temp_req, 0);

            exist_table[old_PPA_zero + i] = NONEXIST;
            exist_table[new_PPA_zero + i] = EXIST;

            algo_req *temp_req2=(algo_req*)malloc(sizeof(algo_req));
            temp_req2->parents = NULL;
            temp_req2->end_req = block_algo_end_req;

            __block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_block, 0, temp_req2, 0);
            free(temp_block);
         }
*/