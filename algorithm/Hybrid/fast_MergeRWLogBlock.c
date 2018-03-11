#include "FAST.h"

/**
 * Function :
 * 
 * Description : 
 * @brief       Merge RW Log Block with Data blocks (O-FAST)
 * @details     First, select victim RW log block. (Algorithm : Round-Robin) \n
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

    // for(int i = )
    /*
    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        // Find up-to-date data in the RW Log Block
        // Set the validness
    }

    for(int i = 0; i < PAGE_PER_BLOCK; i++){
        // if valid
        // 
    }
    */

    return (eNOERROR);
}
