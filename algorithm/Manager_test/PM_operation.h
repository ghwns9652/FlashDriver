#include "PM_blockqueue.h"

void PM_Init(); //initialize selector. uses Block_Manager function.
void PM_Destroy(); // 
uint64_t Alloc_Page(); //uses Block_Manager function.
uint64_t Set_Free(Block** bp, int block_number,int reserved);

B_queue empty_queue;
B_queue reserved_queue;
