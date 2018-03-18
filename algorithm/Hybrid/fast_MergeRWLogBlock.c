#include "FAST.h"

/**
 * Function :
 * 
 * Description : 
 * @brief       Merge RW Log Block with Data blocks (O-FAST)
 * @details     First, select victim RW log block. (Round-Robin) \n
 *              Check validness of each pages in victim block using mapping tables. \n
 *              If there exist another pages with same logical address in mapping tables, \n
 *              set state for pages as invalid. \n
 *              Merge valid pages that have same logical block into original data block \n
 *              until there is no valid page in victim block. \n
 * 
 * @return      Error code for function call
 */
char fast_MergeRWLogBlock(uint32_t log_block)
{ //TODO : Key, Value 다 가지고 와서 그것도 input할 필요가 있음.
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;
    RW_MappingInfo* data = tableInfo->data;

    // TODO : Check Original Paper
    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        if(GET_PAGE_STATE() == VALID){
            fast_FullMerge();
        } 
    }

    Fast_Algorithm.li->trim_block(ADDRESS(rw_log_block, 0), false);

    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        SET_PAGE_STATE(ADDRESS(rw_log_block, i), ERASED);
    }
    SET_BLOCK_STATE(rw_log_block, ERASED);
    // Push RW log block table one elements
    // 2 -> 1, 3 -> 2, ...
    uint32_t* rw_log_block = rw_MappingTable->rw_log_block;
    for(int i = 0; i < NUMBER_OF_RW_LOG_BLOCK - 1; i++){
        rw_log_block[i] = rw_log_block[i+1];
    }
    rw_log_block[NUMBER_OF_RW_LOG_BLOCK - 1] = 0;
    rw_MappingTable->number_of_full_log_block--;
    rw_MappingTable->current_position--;
    rw_MappingTable->offset = 0;
    
    return (eNOERROR);
}

/**
 * @brief       Renew state for pages in RW log block table
 * @details     Find pages that have same logical address in RW log block.
 *              If duplicate pages exists, set state of deuplicate pages as invalid except up-to-date page.
 * @param       logical_address     Logical address which log pages correspond to
 * @param       order               Offset of victim in RW log block
 * @return      Offset of up-to-date page
 */
static int fast_RenewRWLogBlockState(uint32_t logical_address, uint32_t order)
{
    RW_MappingInfo data = tableInfo->rw_MappingTable->data;
    uint32_t victim_position = order;
    uint32_t victim_address = ADDRESS(data[victim_position].logical_block, data[victim_position].physical_block);
    if(GET_PAGE_STATE(victim_address) == INVALID){
        return eALREADYINVALID;
    }
    
    uint32_t current_position = order + 1;
    uint32_t current_address;
    while(current_position < NUMBER_OF_RW_LOG_BLOCK*PAGE_PER_BLOCK){
        current_address = ADDRESS(data[current_position].logical_block, data[current_position].physical_block);
        if(current_address == victim_address){
            SET_PAGE_STATE(victim_address, INVALID); // Check constant
            victim_position = current_position;
        }
        current_position++;
    }
}

/**
 * 
 */
static char fast_FullMerge(int block_number)
{
    RW_MappingInfo* data = tableInfo->rw_MappingTable->data;
    uint32_t* candidates = (uint32_t*)malloc(sizeof(uint32_t*)*PAGE_PER_BLOCK);

    int offset;
    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        if(data[i].logical_block == block_number){
            offset = fast_RenewRWLogBlockState();
            candidates[data[offset].logical_offset] = ADDRESS(data[offset].physical_block, data[offset].physical_offset);
        }
    }

    for(int i = 0; i < PAGE_PER_NUMBER; i++){
        if(GET_PAGE_STATE(ADDRESS(block_number, i) == VALID)){
            candidates[i] = ADDRESS(block_number, i);
        }
    }

    for(int i = 0; i < PAGE_PER_NUMBER; i++){
        src_address = candidates[i];
        dst_address = NULL; //TODO

        params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
        params->parents = req;

        my_req = (algo_req*)malloc(sizeof(algo_req));
        my_req->end_req = FAST_EndRequest;
        my_req->params = (void*)params;

		FAST_Algorithm.li->pull_data(src_address, PAGESIZE, value, 0, my_req, 0);
		FAST_Algorithm.li->push_data(dst_address, PAGESIZE, value, 0, my_req, 0);

        free(my_req);
        free(params);
    }

    FAST_Algorithm.li->trim_block(ADDRESS(block_number, 0), false);

    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        SET_PAGE_STATE(ADDRESS(block_number, i), ERASED);
        SET_PAGE_STATE(ADDRESS(new_data_block, i), VALID);
    }

    SET_BLOCK_STATE(block_number, ERASED);
    SET_BLOCK_STATE(new_data_block, DATA_BLOCK);
}