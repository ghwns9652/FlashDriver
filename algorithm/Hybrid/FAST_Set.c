#include "FAST.h"

/**
 * Function :
 * 
 * Description : 
 * 
 * Returns :
 *  No returns
 * 
 * @TODO    Should optimize Merge opeartion of SW log block to reduce unneccesary writing to RW log block
 */

uint32_t FAST_Set(request *const req)
{
    KEYT                key;
    value_set*          value;
    uint32_t            physical_address;

    key = req->key;
    value = req->value;    

    // Find page to write
    if(fast_AllocDataBlockEntry(key, &physical_address) != eNOERROR){
        if(fast_AllocSWLogBlockEntry(key, &physical_address, req) != eNOERROR){
        	fast_AllocRWLogBlockEntry(key, &physical_address, req);
        }
    }

    printf("Set : %d to %d\n", key, physical_address);
    fast_WritePage(physical_address, req, value, 0);

    return 1;
}
