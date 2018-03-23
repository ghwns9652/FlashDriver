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
//nV_T* numValid_map[_NOB];
//PE_T* PE_map[_NOB];

/* (IGNORE!) Incomplete */
#if 0
struct algorithm __BM={
	.create=BM_create,
	.destroy=BM_destroy,
	.get=BM_get,
	.set=BM_set,
	.remove=BM_remove
};
#endif

/* Initiation of Bad-Block Manager */
int32_t BM_Init()
{
	int32_t bA_Exist;

	printf("BM_Init() Start..\n");
	printf("Start Bad-Block Manager..\n\n");
	
	printf("Read Flash..\n");
	
	/* Here.. READ FUNCTION.. */

	/* Read bA_Exist in Flash, and check whether blockArray exists or not */

	/* bA_Exist is a data of first page in the block for BM. If blockArray exists, then bA_Exist == _BA_EXIST. If not, blockArray doesn't exist */

 	bA_Exist = 12345; // For normal work, it should be read in Flash blocks. 12345 means just 'else'

	printf("Allocate blockArray..");	BM_InitBlock();	printf("..End\n");

	if (bA_Exist == _BA_GOODSTATE){
		/* SSD was turned off normally, so just load blockArray in Blockmanager block */ // Blockmanager block: Flash blocks for BM
		printf("In the past, SSD was normally turned off. Load blockArray..");
		BM_LoadBlock(0);	printf("..End\n");
	}
	/* If the body of 'else if' and 'else' is same, we can merge two condition to 'else' */
	else if (bA_Exist == _BA_BADSTATE){
		/* SSD was turned off abnormally, so scan all flash blocks and make blockArray */
		printf("SSD was abnormally turned off, so there is a consistancy problem.. Scan & Restore blockArray using OOB\n");
	}
	else{
		/* Turn on this SSD for the first time, so initialize blockArray */
		printf("Initialize blockArray..");
		BM_InitBlockArray();
		printf("..End\n");
	}

	printf("Bad Block Check..");	BM_BadBlockCheck();	printf("..End\n");
	printf("Fill pointer maps..");	BM_FillMap();		printf("..End\n");
	
	printf("BM_Init() End!\n");
}


/* Initialization of Block structures */
int32_t BM_InitBlock()
{
	blockArray= (Block*)malloc(sizeof(Block) * _NOB);
	
	numValid_map = (nV_T**)malloc(sizeof(nV_T*) * _NOB);
	PE_map = (PE_T**)malloc(sizeof(PE_T*) * _NOB);
	//numValid_map = (uint8_t**)malloc(sizeof(uint8_t*) * _NOB);
	//PE_map = (uint32_t**)malloc(sizeof(uint32_t*) * _NOB);
	/*	
	for (int i=0; i<_NOB; ++i){
		numValid_map[i] = (nV_T**)malloc(sizeof(nV_T*));
		PE_map[i] = (PE_T**)malloc(sizeof(PE_T*));
	}
	*/
	return(eNOERROR);
}

/* Load data to blockArray */
/*
 * Write each PBA, numValid, etc... to blockArray. 
 */
int32_t BM_LoadBlock(uint32_t PBA_BM) 
{
	printf("Loading Block Data..\n");
	// This functon is a prototype. Suppose that this function is completed.
}


/* Initalize blockArray */
int32_t BM_InitBlockArray()
{
	for (int i=0; i<_NOB; ++i){
		blockArray[i].PBA = i;
		for (int j=0; j<_PPB; ++j)
			blockArray[i].ValidP[j] = BM_INVALIDPAGE;
		blockArray[i].numValid = _PPB;
		blockArray[i].PE_cycle = 0;
		blockArray[i].BAD = _NOTBADSTATE;
	}
}



#if 1
/* Scan data to make blockArray */
/*
 * Scan all flash blocks to fill the data of blockArray
 */
int32_t BM_ScanFlash()
{
	/* Access and Read data in all flash blocks */
	for (int i=0; i<_NOB; ++i){
		BM_ReadBlock(i);

	}
}

int32_t BM_ReadBlock(){//(int32_t PBA){
	/* (Later)Read OOB of PBA block and Fill blockArray */
	/* (IGNORE!) It is incomplete */

	printf("Start BM_ReadBlock. But this function is empty\n");
	#if 0
	int8_t* temp_block = (int8_t*)malloc(_PPB * PAGESIZE);

	for (uint8_t offset = 0; offset < _PPB; ++offset){ // int offset..?
		algo_req* my_req = (algo_req*)malloc(sizeof(algo_req));
		my_req->parents = NULL;
		my_req->end_req = BM_end_req;

		//uint32_t PPA = PBA * _PPB + offset;
		__block.li->pull_data(PBA * _PPB + offset, PAGESIZE, temp_block + (offset*PAGESIZE), 0, my_req, 0); // Question: Format of pull_data to 'Block' is right?
	}
	blockArray[PBA].PBA = PBA;
	#endif
}

#endif

/* Check which blocks are bad */
int32_t BM_BadBlockCheck()
{
	/* This functions should be made with HARDWARE Function */

	for (int i = 0; i < _NOB; ++i) {
		/* HARDWARE Functions to check bad state */
		// Here is HARDWARE Function
		i++;

		//blockArray[i].BAD = _BADSTATE;
	}
}

/* Fill numValid_map, PE_map from blockArray */
int32_t BM_FillMap()
{
	for (int i = 0; i < _NOB; ++i) {
		if (blockArray[i].BAD != _BADSTATE) { /* Does this condition need? Anyway we know the state of BAD.. So we wouldn't need to prevent filling maps */
			numValid_map[i] = &(blockArray[i].numValid);
			PE_map[i] = &(blockArray[i].PE_cycle);

			blockArray[i].ptrNV_data = &(numValid_map[i]);
			blockArray[i].ptrPE_data = &(PE_map[i]);
		}
	}

	return(eNOERROR);


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

