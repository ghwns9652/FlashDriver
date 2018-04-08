#include "PM_operation.h"

extern Block* blockArray;
extern nV_T** numValid_map;
extern B_queue empty_queue;
extern B_queue reserved_queue;
int main(void)
{
	PM_Init();
	/*testing BM::Initialization*/
	printf("component check. %d, %d\n",blockArray[3].PBA, blockArray[2].BAD);
	printf("queue Counter. %d, %d\n",empty_queue.count, reserved_queue.count);
	printf("validP array : %d, %d/ \n",
blockArray[3].ValidP[3],blockArray[3].ValidP[5]);
	
	/*testing BM_operations::invalidation*/
	BM_invalidate_ppa(blockArray, 3);
	BM_invalidate_ppa(blockArray, 4);
	int a = BM_is_invalid_ppa(blockArray, 3);
	int b = BM_is_invalid_ppa(blockArray, 4);
	int c = BM_is_invalid_ppa(blockArray, 5);
	printf("validty check, a:%d,b:%d,c:%d\n",a,b,c);

	/*testing BM_operations::updating*/
	BM_update_block_with_gc(blockArray, 3);
	BM_update_block_with_push(blockArray, 10);
	BM_update_block_with_push(blockArray, 12);
	BM_update_block_with_trim(blockArray, 0);
	printf("PE_cycle: %d\n", blockArray[0].PE_cycle);
	printf("validty of page: %d\n", blockArray[0].ValidP[10]);

	/*testing BM_operations::get_functions*/
	/*give fake info to blockArray[i].numValid) */
	for (int i = 0; i < _NOB ; i++)
	{
		if ( i == 2)
			blockArray[i].numValid = 3;
		else
			blockArray[i].numValid = 10;
	}

	/*get test.*/
	int gc_target = BM_get_gc_victim(blockArray, numValid_map);
	printf("gc_target : %d\n",gc_target);
	/*done*/

	/*testing PM_operations::Alloc_page */
	for (int i = 0; i < 12; i++)
	{
		int current = Alloc_Page();
		printf("milestone : %d\n",current);
	}



	PM_Destroy();
	return 0;





}

