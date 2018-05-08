#include "FAST.h"

/**
 * Function :
 *  fast_SearchSWLogBlock(uint32_t logical_address, uint32_t* physical_address)
 * 
 * Description : 
 *  Search SW log block and translate logical_address to physical_address
 *      if SW log block have corresponding entry.
 *  First, logical_address is diveded into logical_block and offset.
 *  Then, get information about SW log block which logical block corresponding to.
 *  If logical block correspond to SW log block, check offset.
 *  If offset is less than number of stored sector in SW log block,
 *      set physical_address using physical_block stored in SW log block information
 *      and return eNOERROR.
 *  If given logical_address didn't satisfy condition above, return eNOTFOUND.
 * 
 * Returns :
 *  eNOERROR - success to find elements in SW log block
 *  eNOTFOUND - fail to find elements in SW log block
 */

char fast_SearchSWLogBlock(uint32_t logical_address, uint32_t* physical_address)
{
    uint32_t logical_block = BLOCK(logical_address);
    uint32_t offset = OFFSET(logical_address);
    SW_MappingTable* sw_MappingTable = tableInfo->sw_MappingTable;

    uint32_t sw_log_block = sw_MappingTable->data->logical_block;
    uint32_t physical_block = sw_MappingTable->data->sw_log_block;
    uint32_t number_of_stored_sector = sw_MappingTable->data->number_of_stored_sector;

    if(logical_block == sw_log_block && offset < number_of_stored_sector){
        *physical_address = ADDRESS(physical_block, offset);
        return (eNOERROR);
    }
    
    return (eNOTFOUND);
}