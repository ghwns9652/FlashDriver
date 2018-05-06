#include "FAST.h"
#include <limits.h>
/**
 * Function : uint32_t FAST_Create(lower_info* li, algorithm* algo)
 * 
 * Description : 
 *  Initiate FAST FTL.
 *  Allocate memory for mapping table (Block Level, Page Level - SW & RW)
 * 
 *  Sector mapping table for SW (Sequential Write) keeps information 
 *  for physical & logical block number, number of page stored in the log block.
 *  Sector mapping table for RW (Random Write) keeps information
 *  for which sectors are written, the offset of RW log blocks.
 * 
 *  Block mapping table keeps information for data block which logical block corresponds to.
 *  Because we search Log blocks first, we don't need to keep information for log blocks.
 * 
 * Returns :
 *  No returns
 * 
 * Side effects:
 *  Memeory allocation for mapping table
 */

struct algorithm FAST_Algorithm = {
    .create = FAST_Create,
    .destroy = FAST_Destroy,
    .get = FAST_Get,
    .set = FAST_Set,
    .remove = FAST_Remove
};

TableInfo* tableInfo;

char* BLOCK_STATE;
char* PAGE_STATE;

/* Definition of constant value */
uint32_t NUMBER_OF_BLOCK;
uint32_t NUMBER_OF_PAGE;
uint32_t SIZE_OF_KEY_TYPE;
uint32_t SIZE_OF_BLOCK;
uint32_t PAGE_PER_BLOCK;
uint32_t TOTAL_SIZE;

const uint32_t NUMBER_OF_RW_LOG_BLOCK = 15;

const char ERASED = 0;
const char VALID = 1;
const char INVALID = 2;

const char eNOERROR = 0;
const char eNOTFOUND = -1;
const char UNEXPECTED = -2;
const char eNOTWRITTEN = -3;
const char eOVERWRITTED = -4;
const char eUNEXPECTED = -5;
const char eNOTSEQUENTIAL = -6;

const char NIL = -1;

// const char ERASED = 0;
const char DATA_BLOCK = 1;
const char SW_LOG_BLOCK = 2;
const char RW_LOG_BLOCK = 3;

uint32_t NUMBER_OF_DATA_BLOCK;
uint32_t BLOCK_LAST_USED;

uint32_t FAST_Create(lower_info* li, algorithm* algo)
{
    // Definition of Global Varables
    NUMBER_OF_BLOCK = li->NOB;
    NUMBER_OF_PAGE = li->NOP;
    SIZE_OF_KEY_TYPE = li->SOK;
    SIZE_OF_BLOCK = li->SOB;
    PAGE_PER_BLOCK = li->PPB;
    TOTAL_SIZE = li->TS;

    NUMBER_OF_DATA_BLOCK = NUMBER_OF_BLOCK / 2;
    algo->li = li;      /* li means Lower Info */

    // Memory Allocation for Global Variables (Pointer)
    tableInfo = (TableInfo*)malloc(sizeof(TableInfo));

    tableInfo->sw_MappingTable = (SW_MappingTable*)malloc(sizeof(SW_MappingTable));
    tableInfo->rw_MappingTable = (RW_MappingTable*)malloc(sizeof(RW_MappingTable) * NUMBER_OF_RW_LOG_BLOCK);
    tableInfo->block_MappingTable = (Block_MappingTable*)malloc(sizeof(Block_MappingTable));

    tableInfo->sw_MappingTable->data = 
        (SW_MappingInfo*)malloc(sizeof(SW_MappingInfo) * PAGE_PER_BLOCK);
    memset(tableInfo->sw_MappingTable->data, 0, sizeof(SW_MappingInfo) * PAGE_PER_BLOCK);
    tableInfo->sw_MappingTable->data->logical_block = -1;
    tableInfo->sw_MappingTable->data->sw_log_block = -1;

    tableInfo->rw_MappingTable->data = 
        (RW_MappingInfo*)malloc(sizeof(RW_MappingInfo) * (PAGE_PER_BLOCK*NUMBER_OF_RW_LOG_BLOCK));
    memset(tableInfo->rw_MappingTable->data, 0, sizeof(RW_MappingInfo) * (PAGE_PER_BLOCK*NUMBER_OF_RW_LOG_BLOCK));

    tableInfo->block_MappingTable->data = 
        (Block_MappingInfo*)malloc(sizeof(Block_MappingInfo) * NUMBER_OF_DATA_BLOCK);
    memset(tableInfo->block_MappingTable->data, 0, sizeof(Block_MappingInfo) * NUMBER_OF_DATA_BLOCK);

    // Setting block mapping information
    BLOCK_STATE = (char*)malloc(sizeof(char)*NUMBER_OF_BLOCK+1+NUMBER_OF_RW_LOG_BLOCK);
    PAGE_STATE = (char*)malloc(sizeof(char)*NUMBER_OF_PAGE);
    tableInfo->rw_MappingTable->rw_log_block = (int*)malloc(sizeof(int)*NUMBER_OF_RW_LOG_BLOCK);
    tableInfo->rw_MappingTable->current_position = 0;
    tableInfo->rw_MappingTable->number_of_full_log_block = 0;
    tableInfo->rw_MappingTable->offset = 0;

    memset(tableInfo->rw_MappingTable->rw_log_block, -1, sizeof(int)*NUMBER_OF_RW_LOG_BLOCK);
    // Test
    /*for(unsigned int i = 0; i < NUMBER_OF_RW_LOG_BLOCK; i++){
        printf("%d ", tableInfo->rw_MappingTable->rw_log_block[i]);
    }
    printf("\n");
    */
    memset(BLOCK_STATE, ERASED, NUMBER_OF_BLOCK+1+NUMBER_OF_RW_LOG_BLOCK);
    memset(PAGE_STATE, ERASED, NUMBER_OF_PAGE);

    for(unsigned int i = 0; i < NUMBER_OF_DATA_BLOCK; i++){
        tableInfo->block_MappingTable->data[i].physical_block = i;
        SET_BLOCK_STATE(i, DATA_BLOCK);
    }
    tableInfo->sw_MappingTable->data->sw_log_block = NUMBER_OF_DATA_BLOCK;
    SET_BLOCK_STATE(NUMBER_OF_DATA_BLOCK, SW_LOG_BLOCK);

    for(unsigned int i = 0; i < NUMBER_OF_RW_LOG_BLOCK; i++){
        SET_BLOCK_STATE(NUMBER_OF_DATA_BLOCK+1 + i, RW_LOG_BLOCK);
        tableInfo->rw_MappingTable->rw_log_block[i] = NUMBER_OF_DATA_BLOCK+1 + i;
        printf("%d %d\n", i, tableInfo->rw_MappingTable->rw_log_block[i]);
    }
    
    BLOCK_LAST_USED = NUMBER_OF_DATA_BLOCK + NUMBER_OF_RW_LOG_BLOCK;

    printf("FAST FTL Creation Finished!\n");
	printf("Page Per Block : %d\n", PAGE_PER_BLOCK);
	printf("Total_Size : %d\n", TOTAL_SIZE);
	printf("NUMBER_OF_PAGE : %d\n", NUMBER_OF_PAGE);
    printf("NUMBER_OF_BLOCK : %d\n", NUMBER_OF_BLOCK);
    printf("NUMBER_OF_DATA_BLOCK : %d\n", NUMBER_OF_DATA_BLOCK);
    printf("BLOCK_LAST_USED : %d\n", BLOCK_LAST_USED);

    return 1;
}
