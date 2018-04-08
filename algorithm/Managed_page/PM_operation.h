#ifndef _PM_OPERATION_H_
#define _PM_OPERATION_H_

#include "PM_blockqueue.h"

void PM_Init(); //initialize selector. uses Block_Manager function.
void PM_Destroy(); // 
uint64_t Alloc_Page(); //uses Block_Manager function.
uint64_t Set_Free(Block** bp, int block_number,int reserved);

#endif //!_PM_OPERATION_H_
