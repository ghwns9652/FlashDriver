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
{
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;
    RW_MappingInfo* data = tableInfo->data;

    // Update validness for victim block
    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        fast_RenewRWLogBlockState(i);
    }

    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        if(GET_PAGE_STATE()){
            //TODO, Merge two block
        } 
    }

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
