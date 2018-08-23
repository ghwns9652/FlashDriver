// #include "FAST.h"

// uint32_t fast_FindErasedBlock()
// {
//     while(GET_PAGE_STATE(++BLOCK_LAST_USED) != ERASED){
//         BLOCK_LAST_USED = BLOCK_LAST_USED % NUMBER_OF_PAGE;
//     }
    
//     return BLOCK_LAST_USED;
// }