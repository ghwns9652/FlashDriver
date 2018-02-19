#include "FAST.h"

/**
 * Function :
 * 
 * 
 * Description : 
 * 
 * 
 * Returns :
 *  No returns
 */

char fast_SearchRWLogBlock(uint32_t logical_address, uint32_t* physical_address)
{
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;
    RW_MappingInfo* entry;
    // char isFind;
    uint32_t number_of_total_entry = PAGE_PER_BLOCK* NUMBER_OF_RW_LOG_BLOCK;
    uint32_t number_of_written_page = (rw_MappingTable->number_of_full_log_block*PAGE_PER_BLOCK)+rw_MappingTable->offset;

    for (entry=rw_MappingTable->data+(number_of_written_page-1); entry>=rw_MappingTable->data; entry--) {
        if (logical_address == ADDRESS(entry->logical_block, entry->logical_offset)) {
            *physical_address = ADDRESS(entry->physical_block, entry->physical_offset);
            return (eNOERROR);
        }
    }
    /*
    for(entry = rw_MappingTable->data + (number_of_total_entry - 1); entry >= rw_MappingTable->data; entry--){        
        if(logical_address == ADDRESS(entry->logical_block, entry->logical_offset)){
			//printf("Check: %d %d", entry->logical_block, entry->logical_offset);
			//printf("Check: %d %d", entry->physical_block, entry->physical_offset);
            *physical_address = ADDRESS(entry->logical_block, entry->logical_offset);
            return (eNOERROR);
        }
    }
    */
    printf("eNOTFOUND ");
    return (eNOTFOUND);
}

// 갯수를 가지고 그 안에 있는지를 학인하고 리턴함.
