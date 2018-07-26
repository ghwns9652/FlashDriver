#include "FAST.h"

/**
 * @brief
 * @detail
 * @param       logical_block   Block number for victim block
 * @param       req             request from FAST_Set function
 * @return      Error code
 */
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