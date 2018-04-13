#include "FAST.h"

uint32_t BLOCK(uint32_t logical_address)             
        { return logical_address / PAGE_PER_BLOCK; }
uint32_t OFFSET(uint32_t logical_address)            
        { return logical_address % PAGE_PER_BLOCK; }
uint32_t ADDRESS(uint32_t block, uint32_t offset)    
        { return block * PAGE_PER_BLOCK + offset; }
uint32_t BLOCK_TABLE(uint32_t logical_block)
        { return tableInfo->block_MappingTable->data[logical_block].physical_block; }
/*
inline int SW_LOG_BLOCK(uint32_t logical_block)
        { return (logical_block == sw_MappingInfo) ? sw_MappingInfo->physical_block : INVALID; }
inline int RW_LOG_BLOCK(uint32_t logical_block)
        { return ()}
*/
char GET_PAGE_STATE(uint32_t physical_address)
        { return *(PAGE_STATE + physical_address); }
void SET_PAGE_STATE(uint32_t physical_address, char state)
        { *(PAGE_STATE + physical_address) = state; }

char GET_BLOCK_STATE(uint32_t physical_address)
        { return *(BLOCK_STATE + physical_address); }
void SET_BLOCK_STATE(uint32_t physical_address, char state)
        { *(BLOCK_STATE + physical_address) = state; }

uint32_t FIND_ERASED_BLOCK()
{
    while(GET_BLOCK_STATE(BLOCK_LAST_USED) != ERASED){
        BLOCK_LAST_USED = (BLOCK_LAST_USED + 1) % NUMBER_OF_BLOCK;
    }
    return BLOCK_LAST_USED;
}