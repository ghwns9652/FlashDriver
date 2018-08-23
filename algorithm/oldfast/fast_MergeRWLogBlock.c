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
char fast_MergeRWLogBlock(uint32_t log_block, request *const req)
{ //TODO : Key, Value 다 가지고 와서 그것도 input할 필요가 있음.
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;
    RW_MappingInfo* data = rw_MappingTable->data;
    int victim_block = rw_MappingTable->rw_log_block[0];
    int new_rw_log_block;
    int i;
    
    for(i = 0; i < PAGE_PER_BLOCK; i++){
        fast_UpdateRWLogBlockState(ADDRESS(data[i].logical_block, data[i].logical_offset), i);
        if(GET_PAGE_STATE(ADDRESS(victim_block, i)) == VALID){
            fast_FullMerge(data[i].logical_block, req);
        }
    }

    FAST_Algorithm.li->trim_block(ADDRESS(victim_block, 0), false);

    for(i = 0; i < PAGE_PER_BLOCK; i++){
        SET_PAGE_STATE(ADDRESS(victim_block, i), ERASED);
    }
    SET_BLOCK_STATE(victim_block, ERASED);

    // Push RW log block table one elements. e.g. 2 -> 1, 3 -> 2, ...
    int* rw_log_block = rw_MappingTable->rw_log_block;
    for(i = 0; i < NUMBER_OF_RW_LOG_BLOCK - 1; i++){
        rw_log_block[i] = rw_log_block[i+1];
    }
    // new_rw_log_block
    rw_MappingTable->number_of_full_log_block--;
    rw_MappingTable->current_position--;
    rw_MappingTable->offset = 0;

    new_rw_log_block = FIND_ERASED_BLOCK();
    rw_log_block[NUMBER_OF_RW_LOG_BLOCK - 1] = new_rw_log_block;
    SET_BLOCK_STATE(new_rw_log_block, RW_LOG_BLOCK);

    for(i = 0; i < PAGE_PER_BLOCK*(NUMBER_OF_RW_LOG_BLOCK - 1); i++){
        data[i] = data[i + PAGE_PER_BLOCK];
    }
    //memcpy(data, data+PAGE_PER_BLOCK, sizeof(RW_MappingInfo)*PAGE_PER_BLOCK*(NUMBER_OF_RW_LOG_BLOCK - 1));
    printf("Merge RW Log Block\n");
    return (eNOERROR);
}