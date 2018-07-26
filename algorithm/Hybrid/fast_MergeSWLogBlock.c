#include "FAST.h"
/**
 * @brief       Merge Operation for Sequential Write (SW) Log Block (Partial Merge)
 * @details     First, copy valid pages in data block into SW log block.\n
 *              Then, change statement of data block into erased block \n
 *              and statement of SW log block into data block.
 * @param       req     Request from FAST_Set function
 * @return      Error code for function call
 */
// 만약 SW 조건 만족안하는게 있음 그거 넣어야 함.
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

// 지금 가지고 있는 정보 위주로 Erase