#include "FAST.h"

char fast_AllocDataBlockEntry(KEYT key, uint32_t* physical_address)
{
    uint32_t    logical_block;
    uint32_t    offset;
    int         data_block;
    char        state;
    
    // Seperate key into block and offset
    logical_block = BLOCK(key);
    offset = OFFSET(key);

    // Translate logical block to physical block
    data_block = BLOCK_TABLE(logical_block);

    // Set physical_address and state of physical_address using data_block and offset
    *physical_address = ADDRESS(data_block, offset);
    state = GET_PAGE_STATE(*physical_address);
    
    // Set state of physical_address dependent to previous state of physical_address
	if (state == VALID) {
		SET_PAGE_STATE(*physical_address, INVALID);
	}
	else if (state == ERASED) {
		SET_PAGE_STATE(*physical_address, VALID);
	}

    return state;
}

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

char fast_AllocSWLogBlockEntry(KEYT key, request *const req, uint32_t* physical_address)
{
    SW_MappingInfo*     sw_MappingInfo;
    uint32_t            offset;
    int                 block;
    int                 logical_block;
   	int                 sw_log_block;

    sw_MappingInfo = tableInfo->sw_MappingTable->data; 

	// If SW log block is full, switch log block into data block
	if (sw_MappingInfo->number_of_stored_sector == PAGE_PER_BLOCK) {
    	block = BLOCK(key);
		fast_SwitchSWLogBlock();
		sw_MappingInfo->logical_block = BLOCK(key);
	}
	
	// Get basic information from given key and mapping table
	block = BLOCK(key);
	offset = OFFSET(key);
	logical_block = sw_MappingInfo->logical_block;
    sw_log_block = sw_MappingInfo->sw_log_block;

	// If offset is 0, request shouold be stored in first page in SW log block
	if (offset == 0) {
		// If SW log block is not empty, we should merge first.
		if (sw_MappingInfo->number_of_stored_sector != 0) {
        	fast_MergeSWLogBlock(key, req);
		}
		sw_MappingInfo->logical_block = block;
	}
	// If block is equal to logical block for SW log block, we should store request in SW log block
	else if (block == logical_block) {
		// If page is not sequential, merge SW log block with request.
		if (offset != sw_MappingInfo->number_of_stored_sector) {
		    fast_MergeSWLogBlock(key, req);
			sw_MappingInfo->logical_block = -1;
			return eMERGEWITH;
		}
		// If page is sequential, update Mapping information for SW log block
		else {
			sw_MappingInfo->number_of_stored_sector++;
		}
	}
	// If block is not equal to logical block for SW log block, just return eNOTSEQUENTIAL
	else {
		return (eNOTSEQUENTIAL);
	}
    
	// Update state for data block and set physical_address with sw_log_block and offset
	SET_PAGE_STATE(ADDRESS(BLOCK_TABLE(block), offset), INVALID);
	*physical_address = ADDRESS(sw_log_block, offset);

#ifdef DEBUG
	printf("SW Log Block ");
#endif
	return (eNOERROR);
}

char fast_SearchDataBlock(uint32_t logical_address, uint32_t* physical_address)
{
    //SW_MappingTable*    sw_MappingTable;
    //RW_MappingTable*    rw_MappingTable;
    uint32_t logical_block = BLOCK(logical_address);
    uint32_t physical_block = BLOCK_TABLE(logical_block);
    uint32_t offset = OFFSET(logical_address);
    
    *physical_address = ADDRESS(physical_block, offset);

    char state = GET_PAGE_STATE(*physical_address);
    if(state == VALID){
        return (eNOERROR);
    }
    else if(state == ERASED){
        return (eNOTWRITTEN);
    }
    else if(state == INVALID){
        return (eOVERWRITTED);
    }

    return (eUNEXPECTED);
    // Block_MappingTable block_MappingTable = tableInfo->block_MappingTable;

//    char state = GET_STATE(physical_address);

//    return state;
}

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

char fast_SwitchSWLogBlock()
{
	SW_MappingInfo*		sw_MappingInfo;
	Block_MappingInfo*	block_MappingInfo;
	uint32_t 			logical_block;
	uint32_t			data_block;
	uint32_t			sw_log_block;

    sw_MappingInfo 		= tableInfo->sw_MappingTable->data;
    block_MappingInfo 	= tableInfo->block_MappingTable->data;

    logical_block 	= sw_MappingInfo->logical_block;
	data_block 		= BLOCK_TABLE(logical_block);
	sw_log_block 	= sw_MappingInfo->sw_log_block;

	FAST_Algorithm.li->trim_block(ADDRESS(data_block, 0), false);
	SET_BLOCK_STATE(data_block, ERASED);
	for(unsigned int i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
	}

	sw_MappingInfo->number_of_stored_sector = 0;
    sw_MappingInfo->sw_log_block = FIND_ERASED_BLOCK();
	SET_BLOCK_STATE(sw_log_block, DATA_BLOCK);
    SET_BLOCK_STATE(sw_MappingInfo->sw_log_block, SW_LOG_BLOCK);

    block_MappingInfo[logical_block].physical_block = sw_log_block;

    printf("Switch Merge\n");

    return (eNOERROR);
}

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
// @TODO
    return eNOERROR;
}