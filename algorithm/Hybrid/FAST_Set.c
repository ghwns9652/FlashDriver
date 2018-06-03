#include "FAST.h"

/**
 * @brief
 * @detail
 * @param
 * @return
 * 
 * 
 * 
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

    key     = req->key;
    value   = req->value;    

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
