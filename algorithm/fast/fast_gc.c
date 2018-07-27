#include "FAST.h"

char fast_FullMerge(int logical_block, request* const req)
{
    RW_MappingInfo* data = tableInfo->rw_MappingTable->data;
    uint32_t* candidates = (uint32_t*)malloc(sizeof(uint32_t*)*PAGE_PER_BLOCK);
    memset(candidates, -1, sizeof(uint32_t)*PAGE_PER_BLOCK);

    unsigned int src_address;
    unsigned int dst_address;
    value_set* value;
    int i;

    int data_block = BLOCK_TABLE(logical_block);
    
    // Allocate new data block
    int new_data_block = FIND_ERASED_BLOCK();

    // Find valid pages in data block save in buffer
    for(i = 0; i < PAGE_PER_BLOCK; i++){
        if(GET_PAGE_STATE(ADDRESS(data_block, i) == VALID)){
            candidates[i] = ADDRESS(data_block, i);
        }
    }

    // Find up-to-date pages in RW log block by searching backward
    for(i = NUMBER_OF_RW_LOG_BLOCK*PAGE_PER_BLOCK - 1; i >= 0; i--){
        if(data[i].logical_block == logical_block){
            if(candidates[data[i].logical_offset] == -1){
                candidates[data[i].logical_offset] = ADDRESS(data[i].physical_block, data[i].physical_offset);
            }
            SET_PAGE_STATE(ADDRESS(data[i].physical_block, data[i].physical_offset), INVALID);
        }
    }
    
    // Rewrite existing pages into new data block
    for(i = 0; i < PAGE_PER_BLOCK; i++){
        if((src_address = candidates[i]) != -1){
            value = fast_ReadPage(src_address, req, NULL, 1);

            dst_address = ADDRESS(new_data_block, i); //TODO
            fast_WritePage(dst_address, req, value, 1);
        }
        else {
            char state = GET_PAGE_STATE(ADDRESS(data_block, i));
            SET_PAGE_STATE(ADDRESS(new_data_block, i), state);
        }
    }

    FAST_Algorithm.li->trim_block(ADDRESS(logical_block, 0), false);

    for(i = 0; i < PAGE_PER_BLOCK; i++){
        SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
    }

    SET_BLOCK_STATE(data_block, ERASED);
    SET_BLOCK_STATE(new_data_block, DATA_BLOCK);
    tableInfo->block_MappingTable->data[logical_block].physical_block = new_data_block;

    printf("Full Merge\n");
    free(candidates);
    return eNOERROR;
}

char fast_MergeRWLogBlock(uint32_t log_block, request *const req)
{ //TODO : Key, Value 다 가지고 와서 그것도 input할 필요가 있음.
    RW_MappingTable* rw_MappingTable = tableInfo->rw_MappingTable;
    RW_MappingInfo* data = rw_MappingTable->data;
    int victim_block = rw_MappingTable->rw_log_block[0];
    int new_rw_log_block;
    int i;
    
    for(i = 0; i < PAGE_PER_BLOCK; i++){
        fast_UpdateRWLogBlockState(ADDRESS(data[i].logical_block, data[i].logical_offset), i);
        if(GET_PAGE_STATE(ADDRESS(victim_block, i)) == VALID){
            fast_FullMerge(data[i].logical_block, req);
        }
    }

    FAST_Algorithm.li->trim_block(ADDRESS(victim_block, 0), false);

    for(i = 0; i < PAGE_PER_BLOCK; i++){
        SET_PAGE_STATE(ADDRESS(victim_block, i), ERASED);
    }
    SET_BLOCK_STATE(victim_block, ERASED);

    // Push RW log block table one elements. e.g. 2 -> 1, 3 -> 2, ...
    int* rw_log_block = rw_MappingTable->rw_log_block;
    for(i = 0; i < NUMBER_OF_RW_LOG_BLOCK - 1; i++){
        rw_log_block[i] = rw_log_block[i+1];
    }
    // new_rw_log_block
    rw_MappingTable->number_of_full_log_block--;
    rw_MappingTable->current_position--;
    rw_MappingTable->offset = 0;

    new_rw_log_block = FIND_ERASED_BLOCK();
    rw_log_block[NUMBER_OF_RW_LOG_BLOCK - 1] = new_rw_log_block;
    SET_BLOCK_STATE(new_rw_log_block, RW_LOG_BLOCK);

    for(i = 0; i < PAGE_PER_BLOCK*(NUMBER_OF_RW_LOG_BLOCK - 1); i++){
        data[i] = data[i + PAGE_PER_BLOCK];
    }
    //memcpy(data, data+PAGE_PER_BLOCK, sizeof(RW_MappingInfo)*PAGE_PER_BLOCK*(NUMBER_OF_RW_LOG_BLOCK - 1));
    printf("Merge RW Log Block\n");
    return (eNOERROR);
}

char fast_MergeSWLogBlock(KEYT key, request *const req)
{
    SW_MappingInfo*     sw_MappingInfo;
    value_set*          value;
	uint32_t            src_address;
	uint32_t            dst_address;

    uint32_t            logical_block;
    uint32_t            data_block;
    uint32_t            sw_log_block;
    uint32_t            new_sw_log_block;
    
    uint32_t            i;

    sw_MappingInfo = tableInfo->sw_MappingTable->data;

    logical_block = sw_MappingInfo->logical_block;
    data_block = BLOCK_TABLE(sw_MappingInfo->logical_block);
	sw_log_block = sw_MappingInfo->sw_log_block;

	for(i = sw_MappingInfo->number_of_stored_sector; i < PAGE_PER_BLOCK; i++){
        src_address = ADDRESS(BLOCK_TABLE(logical_block), i);
        if (key == src_address) {
            continue;
        }
        else if (GET_PAGE_STATE(src_address) == VALID) {
            value = fast_ReadPage(src_address, req, NULL, 1);
            dst_address = ADDRESS(sw_log_block, i);
            fast_WritePage(dst_address, req, value, 1);
        }
        else if (GET_PAGE_STATE(src_address) == INVALID) {
            dst_address = ADDRESS(sw_log_block, i);
            SET_PAGE_STATE(dst_address, INVALID);
        }
	}

	FAST_Algorithm.li->trim_block(ADDRESS(data_block, 0), false);
	SET_BLOCK_STATE(data_block, ERASED);
	for(i = 0; i < PAGE_PER_BLOCK; i++){
		SET_PAGE_STATE(ADDRESS(data_block, i), ERASED);
	}

    tableInfo->block_MappingTable->data[logical_block].physical_block = sw_log_block;

    new_sw_log_block = FIND_ERASED_BLOCK();
    sw_MappingInfo->sw_log_block = new_sw_log_block;
	sw_MappingInfo->number_of_stored_sector = 0;

	SET_BLOCK_STATE(sw_log_block, DATA_BLOCK);
    SET_BLOCK_STATE(new_sw_log_block, SW_LOG_BLOCK);

    printf("Partial Merge\n");
    return (eNOERROR);
}