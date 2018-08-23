#include "FAST.h"

/**
 * @brief
 * @details
 * @param
 * @return
 * 
 * Function :
 * 
 * Description : 
 * 
 * Returns :
 *  No returns
 * 
 * Side effects :
 *  ???
 * 
 */

void* FAST_EndRequest(algo_req *input)
{
    
    FAST_Parameters* params = (FAST_Parameters*)input->params;

    request* req = params->parents;
    req->end_req(req);

    free(params);
    free(input);
    
    /*
	request *res = input->parents;
	res->end_req(res);

	free(input);
    */
    return NULL;
}