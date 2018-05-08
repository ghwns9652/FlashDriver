#include "FAST.h"

void fast_WritePage(uint32_t address, request *const  req, value_set* value_in, char type)
{
	value_set*          value;
	algo_req*           my_req;
	FAST_Parameters*    params;

    if(type){
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
