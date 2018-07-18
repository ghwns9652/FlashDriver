#ifndef _BM_BLOCK_H_
#define _BM_BLOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../include/container.h"

/* Type of member variable */
typedef int32_t PBA_T;
typedef int32_t IV_T;

typedef struct {
	void *value;
	int my_idx;
} h_node;

typedef struct { // 13 + PPB/8 bytes
	PBA_T PBA;			/* PBA of this block */
	IV_T Invalid;		/* Number of Valid pages in this block*/
	h_node *hn_ptr;
	char type;
} Block;

typedef struct {
	int idx;
	int max_size;
	h_node *body;
} Heap;

typedef struct b_node{
	void *data;
	struct b_node *next;
}b_node;

typedef struct b_queue{
	int size;
	b_node *head;
	b_node *tail;
}b_queue;

/* Macros for finding PBA from PPA */
#define BM_PPA_TO_PBA(PPA)	PPA/_PPB

/* Macros for finding member variables from Block ptr */
#define BM_GETPBA(ptr_block) ((Block*)ptr_block)->PBA
#define BM_GETNUMVALID(ptr_block) ((Block*)ptr_block)->Invalid

int32_t BM_Init(Block**);
void BM_Free(Block*);
Heap* BM_Heap_Init(int max_size);
void BM_Heap_Free(Heap *heap);
h_node* BM_Heap_Insert(Heap *heap, Block *value);
Block* BM_Heap_Get_Max(Heap *heap);
void BM_Queue_Init(b_queue **q);
void BM_Queue_Free(b_queue *q);
void BM_Enqueue(b_queue *q, Block* value);
Block* BM_Dequeue(b_queue *q);

//heap
Heap* heap_init(int max_size);
void heap_free(Heap*);
void max_heapify(Heap*);
void heap_print(Heap*);

//queue
void initqueue(b_queue **q);
void freequeue(b_queue *q);
void enqueue(b_queue *q, void* node);
void* dequeue(b_queue *q);

#endif
