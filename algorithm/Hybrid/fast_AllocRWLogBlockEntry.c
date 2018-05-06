#include "FAST.h"

/**
 * Function :
 * 
 * Description : 
 *  Set physical_address for empty RW Log Block Page.
 *  
 *  First, find empty entry in the table.
 *  Second, initiate table entry using given information.
 *  Third, update mapping table information (How many page are written in the log block).
 *  If needed, trim operation can be done to make erased log block.
 * 
 * Returns :
 *  No returns
 */

// @TODO : Please use Circular Queue
char fast_AllocRWLogBlockEntry(KEYT key, uint32_t* physical_address, request *const req)
{
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;

    char current_position = rw_MappingTable->current_position;
    int number_of_full_log_block = rw_MappingTable->number_of_full_log_block;

    int number_of_written_page = number_of_full_log_block*PAGE_PER_BLOCK + rw_MappingTable->offset;
    RW_MappingInfo* data = rw_MappingTable->data;

    if(rw_MappingTable->number_of_full_log_block == NUMBER_OF_RW_LOG_BLOCK){
        // @TODO: TRIM
    }

    // Update Sector mapping for RW Log BLock
    data[number_of_written_page].logical_block = BLOCK(key);
    data[number_of_written_page].logical_offset = OFFSET(key);
    data[number_of_written_page].physical_block = rw_MappingTable->rw_log_block[current_position]; // 대체 가능함
    data[number_of_written_page].physical_offset = rw_MappingTable->offset; // 대체 가능함?
    data[number_of_written_page].state = VALID;

    rw_MappingTable->offset++;

    if(rw_MappingTable->offset == PAGE_PER_BLOCK){
        rw_MappingTable->offset = 0;
        rw_MappingTable->current_position++;
        rw_MappingTable->number_of_full_log_block++;
    }

    *physical_address = ADDRESS(data[number_of_written_page].physical_block, data[number_of_written_page].physical_offset);
    // @TODO : Should think of memory operation (간략하게나마 구현할 수는 있지만, 최적화되지 않음)
    // 원형 큐를 이용해서 최적화를 할 수 있을 거라 생각됨.

    return eNOERROR;
}