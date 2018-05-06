#include "FAST.h"

/**
 * Function :
 *  char fast_SearchRWLogBlock(uint32_t logical_address, uint32_t* physical_address)
 * 
 * Description : 
 *  Search from last elements of RW log block if there exists corresponding elements.
 *  If there is corresponding elements, return eNOERROR.
 *  If not, return eNOTFOUND
 * 
 * Returns :
 *  eNOERROR - success to find elements in RW log block
 *  eNOTFOUND - fail to find elements in RW log block
 */

char fast_SearchRWLogBlock(uint32_t logical_address, uint32_t* physical_address)
{
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;
    RW_MappingInfo* entry;

    //uint32_t number_of_total_entry = PAGE_PER_BLOCK* NUMBER_OF_RW_LOG_BLOCK;
    uint32_t number_of_written_page = (rw_MappingTable->number_of_full_log_block*PAGE_PER_BLOCK)+rw_MappingTable->offset;

    // Should cast into integer to test correctly (if not, doesn't work properly)
    for (entry = rw_MappingTable->data+(number_of_written_page-1); (int)(entry-rw_MappingTable->data) >= 0; entry--) {    
        if (logical_address == ADDRESS(entry->logical_block, entry->logical_offset)) {
            *physical_address = ADDRESS(entry->physical_block, entry->physical_offset);
            return (eNOERROR);
        }
    }

    return (eNOTFOUND);
}