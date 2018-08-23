#include "FAST.h"

/**
 * @brief       Free all memory used for FAST FTL
 * @detail
 * @param
 * @param
 * 
 * Function :
 * 
 * Description : 
 * 
 * Returns :
 *  No returns
 */

void FAST_Destroy(lower_info* li, algorithm* algo)
{
    free(PAGE_STATE);
    free(BLOCK_STATE);

    free(tableInfo->block_MappingTable->data);
    free(tableInfo->rw_MappingTable->data);
    free(tableInfo->sw_MappingTable->data);

    free(tableInfo->block_MappingTable);
    free(tableInfo->rw_MappingTable);
    free(tableInfo->sw_MappingTable);
    
    free(tableInfo);
}