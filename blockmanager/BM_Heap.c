/* BM_HEAP */

#include "BM_Heap.h"

/*
 * Heap Functions for ptrBlock
 */



void max_heapify_cnt_(Block* ptrBlock[], uint32_t size_, int32_t i)
{
	int32_t l = 2 * i + 1;
	int32_t r = 2 * i + 2;
	int32_t largest;
	int32_t size = (int32_t)size_;

	if (l <= size - 1 && ptrBlock[l]->cnt > ptrBlock[i]->cnt)	largest = l;
	else largest = i;

	if (r <= size - 1 && ptrBlock[r]->cnt > ptrBlock[largest]->cnt)	largest = r; // if, else, if의 과정을 통해 가장 큰 value를 가지는 index를 찾아낸 뒤,
	if (largest != i) {                                          // 만약 부모 노드의 값인 i가 largest가 아니라면 largest과 i를 바꾼다.
																 //SWAP(Block_list[i].cnt, Block_list[largest].cnt);
		//SWAP(ptrBlock[i]->cnt, ptrBlock[largest]->cnt);
		SWAP_BLOCK(ptrBlock[i], ptrBlock[largest]);
		max_heapify_cnt_(ptrBlock, size, largest); // 그 후에는 재귀적으로 반복
	}
}

/* Sorting ptrBlock[] by cnt */
// 애초에 ptrBlock의 배열들은 모두 정렬상태가 다른데.. cnt 기준으로 한번에 Block단위로 SWAP한다고 해서 그게 cnt->PBA 연결이 바로 되나?
void build_max_heap_cnt_(Block* ptrBlock[], uint32_t size)
{
	int32_t i;
	for (i = size / 2; i >= 0; --i) {
		max_heapify_cnt_(ptrBlock, size, i);
	}
}






/* ---------------------------------- */
// Heap 기능 다 구현하기. 아직은 조금밖에 없음
// 지금은 대개 int list를 기준으로 만들어져있지만, 나중에 Blocklist로 바꿀 것

/* Return and Extract(Erase) Maximum value in Max-heap */
uint32_t heap_maximum_extract(int* list, int size)
{
	uint32_t max;
	if (size < 1) {
		ERR(eHEAPUNDERFLOW_BM);
		printf("heap underflow\n");
	}
	max = list[0];
	list[0] = list[size - 1];
	list = (int*)realloc(list, sizeof(int) * --size);
	max_heapify(list, size, 0);
	return max;
}

/* Return Maximum value in Max-heap */
uint32_t heap_maximum(int* list) // int*가 아니라 Block*이어야 한다. 
{
	return list[0]; // return type도 사실은 uint32_t가 아니라 Block
}

/* Return */
Block heap_maximum_cnt(Block* Block_list)
{
	// 아 이게 get victim인가
	return Block_list[111];
}

/*
void build_max_heap_cnt(Block* Block_list, uint32_t size)
{
	uint32_t i;
	for (i = size / 2; i >= 0; --i)
		max_heapify_cnt(Block_list, size, 0);
}
*/

void build_max_heap(int* list, int size)
{
	int i;
	for (i = size / 2; i >= 0; --i)
		max_heapify(list, size, i);
}

/*
void max_heapify_cnt(Block* Block_list, uint32_t size_, int32_t i)
{
	int32_t l = 2 * i + 1;
	int32_t r = 2 * i + 2;
	int32_t largest;
	int32_t size = (int32_t)size_;
	
	if (l <= size - 1 && Block_list[l].cnt > Block_list[i].cnt)	largest = l;
	else largest = i;

	if (r <= size - 1 && Block_list[r].cnt > Block_list[largest].cnt)	largest = r; // if, else, if의 과정을 통해 가장 큰 value를 가지는 index를 찾아낸 뒤,
	if (largest != i) {                                          // 만약 부모 노드의 값인 i가 largest가 아니라면 largest과 i를 바꾼다.
		//SWAP(Block_list[i].cnt, Block_list[largest].cnt);
		SWAP_BLOCK(Block_list[i], Block_list[largest]);
		max_heapify_cnt(Block_list, size, largest); // 그 후에는 재귀적으로 반복
	}
}
*/

void max_heapify(int* list, int count, int i) // max heap으로 fix하기
{
	int l = 2 * i + 1;
	int r = 2 * i + 2;
	int largest;
	if (l <= count - 1 && list[l] > list[i])	largest = l;
	else largest = i;
	if (r <= count - 1 && list[r] > list[largest])	largest = r; // if, else, if의 과정을 통해 가장 큰 value를 가지는 index를 찾아낸 뒤,
	if (largest != i) {                                          // 만약 부모 노드의 값인 i가 largest가 아니라면 largest과 i를 바꾼다.
		SWAP(list[i], list[largest]);
		max_heapify(list, count, largest); // 그 후에는 재귀적으로 반복
	}
}

void heapSort(int* list, int count)
{
	//일단 build_max_heap을 진행
	int i;
	int size = count;
	for (i = count / 2; i >= 0; i--)	max_heapify(list, count, i);  // build_max_heap
																	  //
	for (i = count - 1; i > 0; i--)
	{
		SWAP(list[0], list[i]); // 제일 큰 값과 제일 작은 값 교체
		size--;					// 마지막 값을 heap tree에서 제거
		max_heapify(list, size, 0); // 다시 max heap으로 fix
	}
}



