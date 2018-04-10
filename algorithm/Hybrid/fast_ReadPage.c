#include "FAST.h"

value_set* fast_ReadPage(uint32_t address, request* req, char type)
{
	value_set *value;
	algo_req* my_req;
	FAST_Parameters* params;

    params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
    params->parents = req;
    params->test = -1;

	value = inf_get_valueset(NULL, DMAREAD);
	//printf("%p", value->value);

    my_req = (algo_req*)malloc(sizeof(algo_req));
    my_req->end_req = FAST_EndRequest;
    my_req->params = (void*)params;

	if(type){
		FAST_Algorithm.li->pull_data(address, PAGESIZE, value, 0, assign_pseudo_req(), 0); // Page
	}
	else{
		FAST_Algorithm.li->pull_data(address, PAGESIZE, value, 0, my_req, 0); // Page		
	}

	return value;
}