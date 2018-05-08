#include "FAST.h"

/**
 * @brief       Renew state for pages in RW log block table
 * @details     Find pages that have same logical address in RW log block.
 *              If duplicate pages exists, set state of deuplicate pages as invalid except up-to-date page.
 * @param       logical_address     Logical address which log pages correspond to
 * @param       offset              Offset of victim in RW log block
 * @return      Offset of up-to-date page
 */
/**
 * 갱신하고자 하는 블럭의 주소, 오프셋이 필요함.
 */
char fast_UpdateRWLogBlockState(uint32_t logical_address, uint32_t offset)
{
    RW_MappingInfo* data = tableInfo->rw_MappingTable->data;
    
    uint32_t victim_offset = offset;
    uint32_t victim_address = ADDRESS(data[victim_offset].physical_block, data[victim_offset].physical_block);;

    uint32_t current_offset = offset + 1;
    uint32_t current_address;

    if(GET_PAGE_STATE(victim_address) == INVALID){
        return eALREADYINVALID;
    }
    
    while(current_offset < NUMBER_OF_RW_LOG_BLOCK*PAGE_PER_BLOCK){
        current_address = ADDRESS(data[current_offset].logical_block, data[current_offset].logical_offset);
        if(current_address == logical_address){
            SET_PAGE_STATE(victim_address, INVALID);
            victim_offset = current_offset;
        }
        current_offset++;
    }

    return eNOERROR;
}