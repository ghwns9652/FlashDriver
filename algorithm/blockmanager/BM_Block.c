/* Structure Interface */

#include "BM.h"

int32_t numBlock;
int32_t PagePerBlock;

/* Initiation of Block Manager */
int32_t BM_Init(Block** blockArray)
{
	numBlock = _NOS;
	PagePerBlock = _PPS;

	*blockArray = (Block*)malloc(sizeof(Block) * numBlock);

	/* Initialize blockArray */
	BM_InitBlockArray(*blockArray);

	printf("blockArray Test 0~4\n");
	for (int i=0; i<5; ++i){
		printf("PBA: %d\n", (*blockArray)[i].PBA);
		printf("Invalid: %d\n", (*blockArray)[i].Invalid);
	}
	
	printf("BM_Init() End!\n");
	return 0;
}

/* Initalize blockArray */
int32_t BM_InitBlockArray(Block* blockArray)
{
	int numItem = BM_GetnumItem();

	for (int i=0; i<numBlock; ++i){
		blockArray[i].PBA = i;
		blockArray[i].Invalid = 0;
		blockArray[i].hn_ptr = NULL;
		blockArray[i].type = 0;

		/* Initialization with INVALIDPAGE */
		/* for (int j=0; j<PagePerBlock; ++j)
			blockArray[i].ValidP[j] = BM_INVALIDPAGE;
		memset(blockArray[i].ValidP, BM_INVALIDPAGE, sizeof(ValidP_T)*4); */

		/* Initialization with VALIDPAGE */
		blockArray[i].ValidP = (ValidP_T*)malloc(numItem);
		for (int j=0; j<numItem; ++j)
			blockArray[i].ValidP[j] = BM_VALIDPAGE;
	}
	return 0;
}

/* Shutdown of Block structures */
int32_t BM_Free(Block* blockArray)
{
	for (int i=0; i<numBlock; ++i)
		free(blockArray[i].ValidP);
	free(blockArray);
	return 0;
}

/* Heap Interface Functions */
Heap* BM_Heap_Init(int max_size){
	return heap_init(max_size);
}

void BM_Heap_Free(Heap *heap){
	heap_free(heap);
}

h_node* BM_Heap_Insert(Heap *heap, Block *value){
	if(heap->idx == heap->max_size){
		printf("heap full!\n");
		exit(1);
	}
	heap->body[heap->idx].value = (void*)value;
	h_node *res = &heap->body[heap->idx];
	heap->idx++;
	return res;
}

Block* BM_Heap_Get_Max(Heap *heap){
	Block *first, *res;
	max_heapify(heap);
	res = (Block*)heap->body[0].value;
	res->hn_ptr = NULL;
	heap->body[0].value = heap->body[heap->idx - 1].value;
	heap->body[heap->idx - 1].value = NULL;
	first = (Block*)heap->body[0].value;
	first->hn_ptr = &heap->body[0];
	heap->idx--;
	return res;
}

/* Queue Interface Functions */
void BM_Queue_Init(b_queue **q){
	initqueue(q);
}

void BM_Queue_Free(b_queue *q){
	freequeue(q);
}

void BM_Enqueue(b_queue *q, Block* value){
	enqueue(q, (void*)value);
}

Block* BM_Dequeue(b_queue *q){
	return (Block*)dequeue(q);
}