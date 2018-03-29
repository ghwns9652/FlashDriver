#include "PM_operation.h"

//use this global parameters to count 
int64_t block_position = -1;
int64_t local_page_position = 0;
int64_t reserved_local = 0;
int64_t reserved_block = -1;
int8_t GC_phase = 0;

extern Block* blockArray;
extern nV_T* numValid_map[_NOB];
extern PE_T* PE_map[_NOB];

void Selector_Init()//initializes selector.
{
	// Initializing Block manager is enough for now.
	BM_Init();
}

void Selector_Destroy()
{
	BM_Shutdown();
//destroy Block manager's structure array.
}

uint64_t Giveme_Page();
{
	if (reserved == 0)
	{
		if ((block_position == -1) || (local_page_position == _PPB))
		{
			BINFO* newbie = Dequeue(&empty_queue);
			block_position = newbie->head_ppa /_PPB;
			local_page_position = 1;
			return newbie->head_ppa;
		}//if it is first write or block is full, get another block.
		else if ((GC_phase == 1)&&(!IsEmpty(&empty_queue)))
		{
			BINFO* newbie = Dequeue(&empty_queue);
			block_position = newbie->head_ppa / _PPB;
			return newbie->head_ppa + local_page_position;
		}//if GC is done, and need to select new block for set. 
		else
		{
			uint64_t re = block_position *_PPB + local_page_position;
			local_page_position++;
			return re;
		}//if not, increase local_position && give page num.
	}
	else if (reserved == 1)
	{
		if (reserved_block = -1 || reserved_local == _PPB)
		{
			BINFO* newbie = Dequeue(&reserved_queue);
			reserved_block= newbie->head_ppa /_PPB;
			return newbie->head_ppa;
		}//if it is first time using reserved, get block.
		else
		{
			uint64_t re = reserved_block *_PPB + reserved_local;
			reserved_local++;
			return re;
		}//if not, increase local_position && give page num
	}
}

uint64_t Set_Free(BINFO**bp, int block_number, int reserved)
{
	if (reserved == 0)
		Enqueue(&empty_queue, bp[block_number]);
	else if (reserved == 1)
		Enqueue(&reserved_queue, bp[block_number]);
}//after GC, Enqueue blocks with this function.
