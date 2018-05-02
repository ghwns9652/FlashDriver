/* BM_Main */

#include "BM_Interface.h"
extern Block* blockArray;
extern nV_T** numValid_map;
extern PE_T** PE_map;

int main(void)
{
    printf("********************* Block Manager *********************\n");
    printf("Start main!\n\n");
    BM_Init();
	///BM basic function tests.

	int a = BM_is_valid_ppa(blockArray, 1);	
	BM_validate_ppa(blockArray,1);
	int b = BM_is_valid_ppa(blockArray,1);
	BM_invalidate_ppa(blockArray,1);
	int c = BM_is_valid_ppa(blockArray,1);
	printf("validity fuctions : %d %d %d\n", a,b,c);
	
	blockArray[1].numValid = 3;
	blockArray[2].numValid = 4;
	blockArray[3].numValid = 1;
	//gc target should be block 1.
	printf("numvalid: %d\n",blockArray[0].numValid);
	printf("gc function result : %d\n",
		BM_get_gc_victim(blockArray,numValid_map));
	BM_update_block_with_gc(blockArray,0);
	printf("after update_with_gc: %d\n", blockArray[0].numValid);
	printf("BM_Init End in MAIN!!!\n");

	
	BM_Shutdown();
	printf("BM_Shutdown End in MAIN!!\n");
	return 0;
}
