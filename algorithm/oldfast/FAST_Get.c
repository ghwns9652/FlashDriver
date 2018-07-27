#include "FAST.h"

/**
 * Function : FAST_Get(const request *req)
 * 
 * @brief
 * @detail      Make pull request using information in req. \n
 *              First, convert physical block number in req into logical block number. \n
 *              In this process, we will search log block table first. \n
 *              If data is not located in log block, we will search data block table. \n
 *              (SW Log Block -> RW Log Block -> Data Block) \n
 *              Using converted logical block, make posix_push_data request. \n
 *              ???
 * @param       req
 * @return      1 (Guess No Meaning)
 * 
 * 
 * Description : 
 *  Make pull request using imformation in req.
 *  First, convert physical sector number in req to logical page number.
 *  In this process, we will search log block table first.
 *  If data is not located in log block, we will search data block table. 
 *  Using converted logical block, make posix_push_data request.
 * 
 * Returns :
 *  No returns
 */

uint32_t FAST_Get(request *const req)
{
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
