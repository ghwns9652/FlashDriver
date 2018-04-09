#include "FAST.h"

void fast_WritePage(uint32_t address, value_set* value_to_write, const request *req)
{
	value_set* value;
	algo_req* my_req;
	FAST_Parameters* params;

    params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
    params->parents = req;
    params->test = -1;
    
    my_req = (algo_req*)malloc(sizeof(algo_req));
    my_req->end_req = FAST_EndRequest;
    my_req->params = (void*)params;

    value = (value_set*)malloc(sizeof(value_set));
    *value = *req->value;

	value_set* value_in = inf_get_valueset(value->value, DMAWRITE);
	FAST_Algorithm.li->push_data(address, PAGESIZE, value_in, 0, my_req, 0);	// Page unlaod
	//inf_free_valueset(value, DMAWRITE);
}
