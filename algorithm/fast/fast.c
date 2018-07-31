#include "fast.h"

struct algorithm FAST_Algorithm = {
    .create = FAST_Create,
    .destroy = FAST_Destroy,
    .get = FAST_Get,
    .set = FAST_Set,
    .remove = FAST_Remove
};

int32_t nop_;
int32_t nob_;
int32_t ppb_;

uint32_t FAST_Create(lower_info* li, algorithm* algo){
    nop_ = _NOP;
    nob_ = _NOB;
    ppb_ = _PPS;

    algo->li = li;

    printf("FAST FTL Creation Finished!\n");
	printf("NUMBER_OF_PAGE : %d\n", nop_);
    printf("NUMBER_OF_BLOCK : %d\n", nob_);
    printf("Page Per Block : %d\n", ppb_);
    return 1;
}

void FAST_Destroy(lower_info* li, algorithm* algo){

}

void* FAST_EndRequest(algo_req *input){
    fast_params* params = (fast_params*)input->params;
    request* req = params->parents;

    req->end_req(req);

    free(params);
    free(input);
    return NULL;
}

uint32_t FAST_Set(request *const req){
    KEYT                lpa;
    uint32_t            physical_address;

    lpa = req->key;

    // Allocate address for empty page to write
    if(fast_AllocDataBlockEntry(key, &physical_address) != ERASED){
        if(fast_AllocSWLogBlockEntry(key, &physical_address, req) != eNOERROR){
        	fast_AllocRWLogBlockEntry(key, &physical_address, req);
        }
    }

    // Write value to empty page
    printf("Set : %d to %d\n", key, physical_address);
    fast_WritePage(physical_address, req, value, 0);

    return 1;
}

uint32_t FAST_Get(request *const req){
    uint32_t            key;
    value_set*          value;
    uint32_t            physical_address;

    key = req->key;
    value = req->value;

    if(fast_SearchSWLogBlock(key, &physical_address) == eNOTFOUND){
		if(fast_SearchRWLogBlock(key, &physical_address) == eNOTFOUND){
            fast_SearchDataBlock(key, &physical_address);
        }
    }

    printf("Get : %d to %d \n", key, physical_address);
    fast_ReadPage(physical_address, req, value, 0);

    return 1;
}

uint32_t FAST_Remove(request *const req){
    return true;+
}