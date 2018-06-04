#include "FAST.h"

/**
 * @brief       Set physical_address for empty RW Log Block Page
 * @detail      First, find empty entry in the table. \n
 *              Second, initiate table entry using given information. \n
 *              Third, update mapping table information (How many page are written in the log block). \n
 *              If needed (in case of there is no empty entry), \n
 *              trim operation can be done to make erased RW log block. \n
 * @param       key                 (IN) Logical Address
 * @param       physical_address    (OUT) Logical Address
 * @param       req                 (IN) Request from FAST_Set function
 * @returns     State of the page corresponding to given key
 */

// @TODO : Please use Circular Queue
char fast_AllocRWLogBlockEntry(KEYT key, uint32_t* physical_address, request *const req)
{
    RW_MappingTable*    rw_MappingTable;
    RW_MappingInfo*     data;
    int                 number_of_full_log_block;
    int                 number_of_written_page;
    char                current_position;
    char                state;

    rw_MappingTable = tableInfo->rw_MappingTable;
    data = rw_MappingTable->data;
    number_of_full_log_block = rw_MappingTable->number_of_full_log_block;

    if (rw_MappingTable->number_of_full_log_block == NUMBER_OF_RW_LOG_BLOCK) {
        fast_MergeRWLogBlock(BLOCK(key), req);
        number_of_full_log_block = rw_MappingTable->number_of_full_log_block;
        //current_position = rw_MappingTable->current_position;
        //printf("%d %d %d\n", rw_MappingTable->rw_log_block[0] ,rw_MappingTable->rw_log_block[1], rw_MappingTable->rw_log_block[2]);
        //printf("current position: %d, rw_log_block[current_position] %d\n", current_position, rw_MappingTable->rw_log_block[current_position]);
    }

    number_of_written_page = number_of_full_log_block*PAGE_PER_BLOCK + rw_MappingTable->offset;
    current_position = rw_MappingTable->current_position;

    // Update Sector mapping for RW Log BLock
    data[number_of_written_page].logical_block = BLOCK(key);
    data[number_of_written_page].logical_offset = OFFSET(key);
    data[number_of_written_page].physical_block = rw_MappingTable->rw_log_block[current_position]; // 대체 가능함
    data[number_of_written_page].physical_offset = rw_MappingTable->offset; // 대체 가능함?
    data[number_of_written_page].state = VALID;

    rw_MappingTable->offset++;
    // printf("Check memory reak ");
    if (rw_MappingTable->offset == PAGE_PER_BLOCK) {
        rw_MappingTable->offset = 0;
        rw_MappingTable->current_position++;
        rw_MappingTable->number_of_full_log_block++;
    }

    SET_PAGE_STATE(ADDRESS(BLOCK_TABLE(BLOCK(key)), OFFSET(key)), INVALID);
    *physical_address = ADDRESS(data[number_of_written_page].physical_block, data[number_of_written_page].physical_offset);
    state = GET_PAGE_STATE(*physical_address);
    // @TODO : Should think of memory operation (간략하게나마 구현할 수는 있지만, 최적화되지 않음)
    // 원형 큐를 이용해서 최적화를 할 수 있을 거라 생각됨.

    printf("RW Log Block ");

    return state;
}