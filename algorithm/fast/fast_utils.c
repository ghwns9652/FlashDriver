#include "fast.h"

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
    while (GET_BLOCK_STATE(BLOCK_LAST_USED) != ERASED) {
        BLOCK_LAST_USED = (BLOCK_LAST_USED + 1) % NUMBER_OF_BLOCK;
    }
    return BLOCK_LAST_USED;
}

algo_req* assign_pseudo_req()
{
    algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
    pseudo_my_req->parents = NULL;
    pseudo_my_req->end_req = pseudo_end_req;
    
    return pseudo_my_req;
}

void *pseudo_end_req(algo_req* input)
{
    free(input);
}

value_set* fast_ReadPage(uint32_t address, request *const req, value_set* value, char type)
{
	algo_req* my_req;
	FAST_Parameters* params;

    // printf("Address: %d ", address);

	if(type){
    	value = inf_get_valueset(NULL, DMAREAD);
		FAST_Algorithm.li->pull_data(address, PAGESIZE, value, 0, assign_pseudo_req(), 0); // Page
	}
	else{
        params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
        params->parents = req;

        my_req = (algo_req*)malloc(sizeof(algo_req));
        my_req->end_req = FAST_EndRequest;
        my_req->params = (void*)params;

       	value = inf_get_valueset(req->value->value, DMAREAD);
		FAST_Algorithm.li->pull_data(address, PAGESIZE, value, 0, my_req, 0); // Page		
	}

	return value;
}

void fast_WritePage(uint32_t address, request *const  req, value_set* value_in, char type)
{
	value_set*          value;
	algo_req*           my_req;
	FAST_Parameters*    params;

    if (type) {
    	FAST_Algorithm.li->push_data(address, PAGESIZE, value_in, 0, assign_pseudo_req(), 0);	// Page unlaod
	    inf_free_valueset(value_in, DMAWRITE);        
    }
    else{
        params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
        params->parents = req;
        
        my_req = (algo_req*)malloc(sizeof(algo_req));
        my_req->end_req = FAST_EndRequest;
        my_req->params = (void*)params;

        value = inf_get_valueset(value_in->value, DMAWRITE);
        FAST_Algorithm.li->push_data(address, PAGESIZE, value, 0, my_req, 0);	// Page unlaod
	    inf_free_valueset(value, DMAWRITE);
    }

    SET_PAGE_STATE(address, VALID);
}