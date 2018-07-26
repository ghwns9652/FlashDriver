#include "FAST.h"

/**
 * @brief       Set physical_address for empty RW Log Block Page
 * @detail      First, find empty entry in the table. \n
 *              Second, initiate table entry using given information. \n
 *              Third, update mapping table information (How many page are written in the log block). \n
 *              If needed (in case of there is no empty entry), \n
 *              trim operation can be done to make erased RW log block. \n
 * @param       key                 (IN) Logical Address
 * @param       req                 (IN) Request from FAST_Set function
 * @param       physical_address    (OUT) Translated physical Address
 * @returns     Error code
 * 
 * @todo        This function can be optimized by using circular queue
 */
char fast_AllocRWLogBlockEntry(KEYT key, request *const req, uint32_t* physical_address)
{
    RW_MappingTable*    rw_MappingTable;
    RW_MappingInfo*     data;
    int                 number_of_full_log_block;
    int                 number_of_written_page;
    char                current_position;
    char                state;

    rw_MappingTable = tableInfo->rw_MappingTable;
    data = rw_MappingTable->data;

    // Make erased RW log block if there is no erased page in RW log blocks
    if (rw_MappingTable->number_of_full_log_block == NUMBER_OF_RW_LOG_BLOCK) {
        fast_MergeRWLogBlock(BLOCK(key), req);
    }

    number_of_full_log_block = rw_MappingTable->number_of_full_log_block;
    number_of_written_page = number_of_full_log_block*PAGE_PER_BLOCK + rw_MappingTable->offset;
    current_position = rw_MappingTable->current_position;

    // Update Page mapping for RW Log BLock
    data[number_of_written_page].logical_block = BLOCK(key);
    data[number_of_written_page].logical_offset = OFFSET(key);
    data[number_of_written_page].physical_block = rw_MappingTable->rw_log_block[current_position]; // 대체 가능함
    data[number_of_written_page].physical_offset = rw_MappingTable->offset; // 대체 가능함?
    data[number_of_written_page].state = VALID;

    // Update mapping table for RW log block
    rw_MappingTable->offset++;
    if (rw_MappingTable->offset == PAGE_PER_BLOCK) {
        rw_MappingTable->offset = 0;
        rw_MappingTable->current_position++;
        rw_MappingTable->number_of_full_log_block++;
    }

    // Set page state of data block into invalid
    SET_PAGE_STATE(ADDRESS(BLOCK_TABLE(BLOCK(key)), OFFSET(key)), INVALID);
    
    // Set erased page in RW log block to physical_address and return state of physical_address
    *physical_address = ADDRESS(data[number_of_written_page].physical_block, data[number_of_written_page].physical_offset);
    state = GET_PAGE_STATE(*physical_address);

#ifdef DEBUG
    printf("RW Log Block ");
#endif
    return eNOERROR;
}