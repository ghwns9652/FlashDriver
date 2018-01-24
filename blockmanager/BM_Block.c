/* BM_BLOCK */

#include "BM_Block.h"



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

/* PBA 기준으로 PBA_array를 정렬하는 함수 */
/*
 * PBA
 */






/* Page Validity Bit Functions */
void BM_bitset(Block targetBlock, uint8_t offset)
{

}






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
		printf("arr[%d]->cnt: %d\n", i, arr[i]->cnt);
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
void print_arr_bit(Block* arr[], uint32_t size)
{
	printf("[bit]\n");
	for (int i = 0; i < (int32_t)size; ++i) {
		for (int j = 0; j < 4; ++j) {
			printf("%d ", arr[i]->bit[j]);
		}
		//printf("%d ", arr[i]->bit);
	}
	printf("\n");
}void print_arr_cnt(Block* arr[], uint32_t size)
{
	printf("[cnt]\n");
	for (int i = 0; i < (int32_t)size; ++i) {
		printf("%d ", arr[i]->cnt);
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
void print_arr_all(Block* arr[], uint32_t size) // PBA, bit, cnt, P/E cycle
{
	printf("------------------------------------\n");
	printf("size: %d\n", size);
	printf("Print PBA, bit, cnt, P/E cycle..\n");
	print_arr_PBA(arr, size);
	print_arr_bit(arr, size);
	print_arr_cnt(arr, size);
	print_arr_PE(arr, size);
	printf("------------------------------------\n");
	printf("\n");
}