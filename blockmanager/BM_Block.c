/* BM_BLOCK */

#include "BM_Block.h"

/* 
 * (README)
 * This File(Block.c) is incomplete.
 * You should consider the hardware function is not provided yet!

 * (2018.07.01) Reduce useless features 
 */


/* Declaration of Data Structures */
Block* blockArray;
Block** numValid_map;
Block** PE_map; // 이따 한번에 지우자

/* Initiation of Bad-Block Manager */
int32_t BM_Init()
{
	//int32_t bA_Exist;

	printf("BM_Init() Start..\n");
	printf("Start Block Manager..\n\n");
	printf("Allocate blockArray..");	BM_InitBlock();	printf("..End\n");

	/* Turn on this SSD for the first time, so initialize blockArray */
	printf("Initialize blockArray..");
	BM_InitBlockArray();
	printf("..End\n");

	printf("Fill pointer maps..");	BM_FillMap();		printf("..End\n");

	printf("Test 0~4\n");
	for (int i=0; i<5; ++i){
		printf("PBA: %d\n", blockArray[i].PBA);
		printf("numValid: %d\n", blockArray[i].numValid);
	}
	
	printf("BM_Init() End!\n");
	return 0;
}


/* Initialization of Block structures */
int32_t BM_InitBlock()
{
	blockArray = (Block*)malloc(sizeof(Block) * _NOB);
	
	numValid_map = (Block**)malloc(sizeof(Block*) * _NOB);
	PE_map = (Block**)malloc(sizeof(Block*) * _NOB);
	return(eNOERROR);
}


/* Initalize blockArray */
int32_t BM_InitBlockArray()
{
	int numItem = BM_GetnumItem();

	for (int i=0; i<_NOB; ++i){
		blockArray[i].PBA = i;

		/* Initialization with INVALIDPAGE */
		/* for (int j=0; j<_PPB; ++j)
			blockArray[i].ValidP[j] = BM_INVALIDPAGE;
		memset(blockArray[i].ValidP, BM_INVALIDPAGE, sizeof(ValidP_T)*4); */

		/* Initialization with VALIDPAGE */
		blockArray[i].ValidP = (ValidP_T*)malloc(numItem);
		for (int j=0; j<numItem; ++j)
			blockArray[i].ValidP[j] = BM_VALIDPAGE;

		blockArray[i].numValid = _PPB;
		blockArray[i].LastOffset = 0;
	}
	return 0;
}

/* Fill numValid_map, PE_map from blockArray */
int32_t BM_FillMap()
{
	for (int i = 0; i < _NOB; ++i) {
		numValid_map[i] = &(blockArray[i]);
		PE_map[i] = &(blockArray[i]);

		blockArray[i].ptrNV_data = (void**)(&(numValid_map[i]));
#if 0
		if (blockArray[i].BAD != _BADSTATE) { /* Does this condition need? Anyway we know the state of BAD.. So we wouldn't need to prevent filling maps */
			numValid_map[i] = &(blockArray[i]);
			PE_map[i] = &(blockArray[i]);

			blockArray[i].ptrNV_data = (void**)(&(numValid_map[i]));
			blockArray[i].ptrPE_data = (void**)(&(PE_map[i]));
		}
		else {
			blockArray[i].numValid = -1;
			blockArray[i].PE_cycle = 0xffffffff;
			numValid_map[i] = NULL;
			PE_map[i] = NULL;
		}
#endif

	}

	return(eNOERROR);


}



/* Shutdown of Block structures */
int32_t BM_Shutdown()
{
	// blockArray would be saved in some flash blocks. 

	/* PUSH blockArray, numValid_map, PE_map to Flash */ 
	/* Come Here! */

	for (int i=0; i<_NOB; ++i)
		free(blockArray[i].ValidP);
	free(blockArray);
	free(numValid_map);
	free(PE_map);

	return 0;
}

