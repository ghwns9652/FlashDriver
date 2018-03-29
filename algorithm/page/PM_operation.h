#include "PM_blockqueue.h"
#include "BM_Interface.h"

void Selector_Init(BINFO** bp); //initialize selector. uses Block_Manager function.
void Selector_Destroy(); // 
uint64_t Giveme_Page(int reserved); //uses Block_Manager function.
uint64_t Set_Free(BINFO** bp, int block_number, int reserved);

B_queue empty_queue;
B_queue reserved_queue;

