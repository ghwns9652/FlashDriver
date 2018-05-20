#include "FAST.h"

/**
 * @brief		Allocate empty block in SW Log Block
 * @details		Branch 1: If offset if 0 \n
 * 				 true : Merge SW Log Block and make allocation with given key \n
 * 				 false: Go to Branch 2 \n
 * 				Branch 2: If block number is same \n
 * 				 true : Go to Branch 3 \n
 * 				 false: Return eNOTSEQUENTIAL \n
 * 				Branch 3: If offset is sequential \n
 * 				 true : Allocate SW log block entry \n
 * 				 false: Return eNOTSEQUENTIAL \n
 * @param		key					TODO
 * @param		physical_address	TODO
 * @param		req					TODO
 * @return		Error code
 */
char fast_AllocSWLogBlockEntry(KEYT key, uint32_t* physical_address, request *const req)
{
    //int logical_block;
    SW_MappingInfo*     sw_MappingInfo;
    uint32_t            offset;
    int                 block;
    int                 logical_block;
   	int                 sw_log_block;

    sw_MappingInfo = tableInfo->sw_MappingTable->data; 
	block = BLOCK(key);
	offset = OFFSET(key);
    logical_block = sw_MappingInfo->logical_block;
    sw_log_block = sw_MappingInfo->sw_log_block;
   	
	if (sw_MappingInfo->number_of_stored_sector == PAGE_PER_BLOCK) {
		fast_SwitchSWLogBlock();
        sw_log_block = sw_MappingInfo->sw_log_block;
        logical_block = sw_MappingInfo->logical_block;
	}
	
	if (offset == 0 && GET_PAGE_STATE(ADDRESS(sw_log_block, 0)) != ERASED) {
        fast_MergeSWLogBlock(req);
        sw_log_block = sw_MappingInfo->sw_log_block;
	}
	else if (block == logical_block) {
		if (offset != sw_MappingInfo->number_of_stored_sector) {
		    fast_MergeSWLogBlock(req);
			return (eNOTSEQUENTIAL);
		}
	}
	else {
		return (eNOTSEQUENTIAL);
	}
    
	*physical_address = ADDRESS(sw_MappingInfo->sw_log_block, offset);
	sw_MappingInfo->number_of_stored_sector++;

	printf("SW Log Block ");
	return (eNOERROR);
}
