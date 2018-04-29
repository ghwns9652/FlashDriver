/* BM_BLOCK */

#include "BM_Block.h"


/* 
 * (README)
 * This File(Block.c) is incomplete.
 * You should consider the hardware function is not provided yet!
 */


/* Declaration of Data Structures */
Block* blockArray;
nV_T** numValid_map;
PE_T** PE_map;


/* Initiation of Bad-Block Manager */
int32_t BM_Init()
{
	printf("Totalsize : %ld\n", (long int)TOTALSIZE);
	printf("Pagesize : %d\n", PAGESIZE);
	printf("NOB : %ld\n",(long int)_NOB);
	int32_t bA_Exist;
	printf("BM_Init() Start..\n");
	printf("Start Bad-Block Manager..\n\n");	
	/* Here.. READ FUNCTION.. */

	printf("Allocate blockArray..");	BM_InitBlock();	printf("..End\n");
	printf("Initialize blockArray..");  BM_InitBlockArray(); printf("..End\n");

	printf("Test 0~4\n");
	for (int i=0; i<5; ++i)
	{
		printf("PBA: %d\n", blockArray[i].PBA);
		printf("numValid: %d\n", blockArray[i].numValid);
		printf("PE_cycle: %d\n", blockArray[i].PE_cycle);
		printf("v_PBA: %d\n", blockArray[i].v_PBA);
		printf("o_PBA: %d\n", blockArray[i].o_PBA);
	}

	
	printf("BM_Init() End!\n");
}


/* Initialization of Block structures */
int32_t BM_InitBlock()
{
	long int NOB = _NOB;
	blockArray= (Block*)malloc(sizeof(Block) * );	
	numValid_map = (nV_T**)malloc(sizeof(nV_T*) * NOB);
	PE_map = (PE_T**)malloc(sizeof(PE_T*) * NOB);
	return(eNOERROR);
}

/* Load data to blockArray */
/*
 * Write each PBA, numValid, etc... to blockArray. 
 */


/* Initalize blockArray */
int32_t BM_InitBlockArray()
{
	for (long int i=0; i<_NOB; ++i){
		blockArray[i].PBA = i;
		for (long int j=0; j<_PPB; ++j)
		{
			printf("entered loop2");
			blockArray[i].ValidP[j] = 0;
		}
		blockArray[i].numValid = _PPB;
		blockArray[i].PE_cycle = 0;
		blockArray[i].BAD = 0;
		blockArray[i].v_PBA = -1; // -1
		blockArray[i].o_PBA = -1;
	}
}

/* Shutdown of Block structures */
int32_t BM_Shutdown()
{
	// blockArray would be saved in some flash blocks. 

	/* PUSH blockArray, numValid_map, PE_map to Flash */ 
	/* Come Here! */

	free(blockArray);
	free(numValid_map);
	free(PE_map);
	/*
	for (int i=0; i<_NOB; ++i){
		free(numValid_map[i]);
		free(PE_map[i]);
	}
	*/
	//free(numValid_map);
	//free(PE_map);
}

