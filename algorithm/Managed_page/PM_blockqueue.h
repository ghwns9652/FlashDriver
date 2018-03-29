#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/settings.h"
#include "BM_Interface.h"

//this would be the queue structure for Page_selector
//use linked list to build queue.

typedef struct node {
	Block* block;
	struct node *next;
}node;
//each node has pointer for Block and another pointer to next node.

typedef struct Block_queue
{
	node *front;
	node *rear;
	int count;
}B_queue;
//structure for queue managing.

void Init_Bqueue(B_queue *queue);
int IsEmpty(B_queue *queue);
void Enqueue(B_queue *queue, block* new_node);
block* Dequeue(B_queue *queue);

