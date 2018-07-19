#ifndef _BLOCK_QUEUE_H_
#define _BLOCK_QUEUE_H_


typedef struct _Node{
	int data;
	struct _Node *next;
	struct _Node *prev;
} Node_t;

typedef struct _Queue {
	Node_t* front;
	Node_t* rear;
	int count;
} Queue_t;

void InitQueue(Queue_t* queue);
int IsEmptyQ(Queue_t* queue);
void Enqueue(Queue_t* queue, int data);
int Dequeue(Queue_t* queue);

void UpdateHit(Queue_t* queue, int data);
void DeleteNode(Queue_t* queue, int data);


#endif // !_BLOCK_QUEUE_H_
