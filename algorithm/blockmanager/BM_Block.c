#include "BM.h"

/* 
 * (README)
 * This File(Block.c) is incomplete.
 * You should consider the hardware function is not provided yet!

 * (2018.07.01) Reduce useless features 
 */

/* Declaration of Data Structures */

/* Initiation of Bad-Block Manager */
int32_t BM_Init(Block **blockarray){
	int32_t nob = _NOS;
	*blockarray = (Block*)malloc(sizeof(Block) * nob);
	for (int i = 0; i < nob; i++){
		(*blockarray)[i].PBA = i;
		(*blockarray)[i].Invalid = 0;
		(*blockarray)[i].hn_ptr = NULL;
		(*blockarray)[i].type = 0;
	}
	return 0;
}

void BM_Free(Block *blockarray){
	free(blockarray);
}

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
	if(heap->idx != 1){
		heap->body[0].value = heap->body[heap->idx - 1].value;
		heap->body[heap->idx - 1].value = NULL;
		first = (Block*)heap->body[0].value;
		first->hn_ptr = &heap->body[0];
	}
	heap->idx--;
	return res;
}

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
