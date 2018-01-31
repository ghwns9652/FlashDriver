/* BM_BLOCK */

#include "BM_Block.h"


/* Declaration of Data Structures */
Block* blockArray;
uint8_t* numValid_map[NOB];
uint32_t* PE_map[NOB];

/* Initiation of Bad-Block Manager */
int32_t BM_Init()
{
	printf("Start Bad-Block Manager..\n");
	printf("BM_InitBlock..\n");		BM_InitBlock();
	printf("BM_LoadBlock..\n");		BM_LoadBlock(0);
	printf("BM_FillMap..\n");		BM_FillMap(blockArray, numValid_map, PE_map);

}

/* Initialization of Block structures */
int32_t BM_InitBlock()
{
	blockArray= (Block*)malloc(sizeof(Block) * NOB);

	
	//numValid_map = (uint8_t**)malloc(sizeof(uint8_t*) * NOB);
	//PE_map = (uint32_t**)malloc(sizeof(uint32_t*) * NOB);
	
	return(eNOERROR);
}

/* Load data to blockArray */
/*
 * Write each PBA, numValid, etc... to blockArray. 
 * 어디에서 불러와야 하지? ptr 2개는 나중에 만드는 거라고 쳐도 block 데이터를 읽어와서 PBA, PE_cycle 등을 알아야 하는데..
 */
int32_t BM_LoadBlock(int32_t somewhere)
{
	printf("Loading Block Data..\n");
	// 일단 이 함수가 완성되어있다고 가정하고 다른 함수를 먼저 만들자...

}


/* Fill numValid_map, PE_map from blockArray */
int32_t BM_FillMap(Block* blockArray, uint8_t** numValid_map, uint32_t** PE_map)
{
	for (int i = 0; i < NOB; ++i) {
		numValid_map[i] = &(blockArray[i].numValid);
		PE_map[i] = &(blockArray[i].PE_cycle);

		blockArray[i].ptrNV_data = &(numValid_map[i]);
		blockArray[i].ptrPE_data = &(PE_map[i]);
	}

	return(eNOERROR);


}

/* Check which blocks are bad */
int32_t BM_BadBlockCheck(Block* blockArray)
{
	/* This functions should be made with HARDWARE Function */

}


/* Shutdown of Block structures */
int32_t BM_Shutdown(Block* blockArray, uint8_t** numValid_map, uint32_t** PE_map)
{
	// 아 어딘가에 저장해야 하는데..... Flash 내의 특정 block들을 dedicated block으로 두어서 거기에 하는 방식으로 하게 될 것 같다.
	// 다만.. 기존의 FTL이나 wear-leveling 과정과 꼬이지 않도록 해야 할 것. 나중에 논의될 것

	free(blockArray);
	//free(numValid_map);
	//free(PE_map);
}






#if 0
/* PPA를 가지고 그에 해당하는 Block이 Block_list에서 어디 있는건지 찾는 함수 */
int32_t BM_Find_BlockPlace_by_PPA(Block* ptrBlock[], uint32_t size, uint32_t PPA)
{
	uint32_t PBA = BM_PBA_TO_PPA(PPA);

	/* PBA를 가지고 Block_list에서 검색하기 */
	int32_t Block_index = BM_search_PBA(ptrBlock, size, PBA);

	// 굳이 이렇게 나눠놓을 필요 없이 간단하게 return BM_search_PBA() 로 해도 되고 BM_search_PBA를 굳이 함수로 뺄 필요도 없을거같긴 하다
	return Block_index;
}


/* PBA를 가지고 Block_list에서 검색하기 */
int32_t BM_search_PBA(Block* ptrBlock[], uint32_t size, uint32_t PBA)
{
	// Heap에서 특정 값을 찾기 위해 할 수 있는 다른 방법은 없나? 지금은 linear search인데
	// 애초에 빨리 찾기 위해서 SORTED ARRAY를 만들 생각이었을텐데 어떻게 하지? 애초에 Block_list가 PBA 기준으로 정렬되어있도록 만드나?
	// 아마 어떠한 ARRAY를 PBA 기준으로 정렬하는 함수를 만들어야 할 것 같다.
	int32_t i = 0;
	for (; i < (int32_t)size; ++i) {
		if (ptrBlock[i]->PBA == PBA)
			return i;
	}
	ERR(ePBASEARCH_BM);
}
#endif // 0












/* Common useful functions for block */
void print_arr(int32_t *arr, uint32_t size)
{
	int32_t i = 0;
	for (; i < (int32_t)size; ++i)
		printf("%d ", arr[i]);
	printf("\n");
}

void print_arr2(Block* arr[], uint32_t size)
{
	for (int i = 0; i < 10; ++i) {
		printf("arr[%d]->PBA: %d\n", i, arr[i]->PBA);
		printf("arr[%d]->cnt: %d\n", i, arr[i]->numValid);
		printf("arr[%d]->PE_cycle: %d\n", i, arr[i]->PE_cycle);
	}
}

void print_arr_PBA(Block* arr[], uint32_t size)
{
	printf("[PBA]\n");
	for (int i = 0; i < (int32_t)size; ++i) {
		printf("%d ", arr[i]->PBA);
	}
	printf("\n");
}
void print_arr_ValidP(Block* arr[], uint32_t size)
{
	printf("[ValidP]\n");
	for (int i = 0; i < (int32_t)size; ++i) {
		for (int j = 0; j < 4; ++j) {
			printf("%d ", arr[i]->ValidP[j]);
		}
		//printf("%d ", arr[i]->ValidP);
	}
	printf("\n");
}void print_arr_cnt(Block* arr[], uint32_t size)
{
	printf("[cnt]\n");
	for (int i = 0; i < (int32_t)size; ++i) {
		printf("%d ", arr[i]->numValid);
	}
	printf("\n");
}void print_arr_PE(Block* arr[], uint32_t size)
{
	printf("[PE]\n");
	for (int i = 0; i < (int32_t)size; ++i) {
		printf("%d ", arr[i]->PE_cycle);
	}
	printf("\n");
}
void print_arr_all(Block* arr[], uint32_t size) // PBA, ValidP, cnt, P/E cycle
{
	printf("------------------------------------\n");
	printf("size: %d\n", size);
	printf("Print PBA, ValidP, cnt, P/E cycle..\n");
	print_arr_PBA(arr, size);
	print_arr_ValidP(arr, size);
	print_arr_cnt(arr, size);
	print_arr_PE(arr, size);
	printf("------------------------------------\n");
	printf("\n");
}