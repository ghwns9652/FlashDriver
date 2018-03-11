#include "FAST.h"

/**
 * Function :
 * 
 * Description : 
 * 
 * Returns :
 *  No returns
 */

uint32_t FAST_Set(request* const req)
{
    FAST_Parameters*    params;
    algo_req*           my_req;
    KEYT                key;
    value_set*          value;
    uint32_t            physical_address;

    key = req->key;
    value = req->value;    

    // Find page to write
    if(fast_AllocDataBlockEntry(key, &physical_address) != eNOERROR){
        if(fast_AllocSWLogBlockEntry(key, &physical_address) != eNOERROR){
        	fast_AllocRWLogBlockEntry(key, &physical_address);
        }
    }

    printf("Set : %d to %d\n", key, physical_address);
    //printf("Set : %d to %d", key, physical_address);

    //Push data using translated address
    params = (FAST_Parameters*)malloc(sizeof(FAST_Parameters));
    params->parents = req;
    
    my_req = (algo_req*)malloc(sizeof(algo_req));
    my_req->end_req = FAST_EndRequest;
    my_req->params = (void*)params;

    FAST_Algorithm.li->push_data(physical_address, PAGESIZE, value, 0, my_req, 0);
}
