/* BM_Main */

#include "BM_Interface.h"

//extern Block* blockArray;
//extern Block** numValid_map;
//extern Block** PE_map;

int main(void)
{
    printf("********************* Block Manager *********************\n");
    printf("Start main!\n\n");
    BM_Init();



	printf("BM_Init End in MAIN!!!\n");

	/* Test */
	blockArray[1].numValid = 3;
	blockArray[2].numValid = 4;
	blockArray[3].numValid = 1;
	for (int i=0; i<4; i++)
		printf("blockArray[%d].numValid = %d\n", i, blockArray[i].numValid);

	printf("gc function result: %d\n", BM_get_gc_victim(blockArray, numValid_map));

	
	BM_Shutdown();
	printf("BM_Shutdown End in MAIN!!\n");
	return 0;
}
