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
 * 				 false: Merge SW log block with given key and req and return eMERGEWITH \n
 * @param		key					(IN) Logical address of I/O request
 * @param		req					(IN) Request from FAST_Set function
 * @param		physical_address	(OUT) Translated physical address
 * @return		Error code
 * 
 * @todo		Modify merge operation
 * @todo		Variable name can make confusion
 */
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
