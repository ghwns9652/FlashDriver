#include "PM_operation.h"

//use this global parameters to count 
int64_t block_position = -1;
int64_t local_page_position = 0;
int64_t reserved_local = 0;
int64_t reserved_block = -1;
int8_t GC_phase = 0;
B_queue empty_queue;
B_queue reserved_queue;

extern Block* blockArray;
extern nV_T** numValid_map;
extern PE_T** PE_map;

void PM_Init()//initializes selector.
{
	// Initialize Block manager..
	BM_Init();
	int OP_builder = 0;//Overprovision build init value.
	int OP_area = 1; //number of Overprovision block.
	Init_Bqueue(&reserved_queue);
	Init_Bqueue(&empty_queue);
	for (int i = 0; i <_NOB; i++)//for all blocks,
	{
		if ((OP_builder < OP_area) && (blockArray[i].BAD == _NOTBADSTATE))
		{
			Enqueue(&reserved_queue, &blockArray[i]);
			OP_builder++;
		}//build OP area.
		else if ((OP_builder >= OP_area) && (blockArray[i].BAD == _NOTBADSTATE))
		{
			printf("we've added empty_b\n");
			Enqueue(&empty_queue, &blockArray[i]);//build empty area.
		}
	}
}

void PM_Destroy()
{
	int current_empty = empty_queue.count;
	int current_reserved = reserved_queue.count;
	for (int i = 0; i < current_empty; i++)
	{
		Dequeue(&empty_queue);
	}
	for (int i = 0; i < current_reserved; i++)
	{
		printf("dequeued from reserved_q\n");
		Dequeue(&reserved_queue);
	}
//destroy queue of blocks.
	BM_Shutdown();
//destroy Block manager's structure array.
}

uint64_t Alloc_Page()//allocates available physical page sequentially.
{					 //only use empty_queue.
	if ((block_position == -1) || (local_page_position == _PPB))
	{
		printf("entered if\n");
		Block* newbie = Dequeue(&empty_queue);
		block_position = newbie->PBA;
		local_page_position = 1;
		printf("new block's PBA is %d\n",newbie->PBA);
		return newbie->PBA * _PPB;
	}//if it is first write or block is full, get another block.
	else if ((GC_phase == 1)&&(!IsEmpty(&empty_queue)))
	{
		Block* newbie = Dequeue(&empty_queue);
		block_position = newbie->PBA;
		return (newbie->PBA * _PPB) + local_page_position;
	}//if GC is done, select new block for set, with given offset for valid dat 
	else
	{
		uint64_t re = block_position *_PPB + local_page_position;
		local_page_position++;
		return re;
	}//if not, increase local_position && give page num.
	
}

uint64_t Set_Free(Block**bp, int block_number, int reserved)
{
	if (reserved == 0)
		Enqueue(&empty_queue, bp[block_number]);
	else if (reserved == 1)
		Enqueue(&reserved_queue, bp[block_number]);
}//after GC, Enqueue blocks with this function.
