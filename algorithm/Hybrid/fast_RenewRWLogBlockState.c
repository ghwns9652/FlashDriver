#include "FAST."

/**
 * @brief       Renew state for pages in RW log block table
 * @details     Find pages that have same logical address in RW log block.
 *              If duplicate pages exists, set state of deuplicate pages as invalid except up-to-date page.
 * @param       logical_address     Logical address which log pages correspond to
 * @param       order               Offset of victim in RW log block
 * @return      Error code for function call
 */

char fast_RenewRWLogBlockState(uint32_t logical_address, uint32_t order)
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