#include <stdio.h>
#include <stdlib.h>
#include "block_queue.h"

/* General Singly Linked Queue */

void InitQueue(Queue_t* queue)
{
	queue->front = NULL;
	queue->rear = NULL;
	queue->count = 0;
}

int IsEmptyQ(Queue_t* queue)
{
	return (queue->count == 0);
	//return (queue->front == queue->rear);
}

void Enqueue(Queue_t* queue, int data)
{
	Node_t* now = (Node_t*)malloc(sizeof(Node_t));
	now->data = data;
	now->next = NULL;

	if (IsEmptyQ(queue)) {
		queue->front = now;
		//queue->rear = now;
	}
	else {
		queue->rear->next = now;
		//now->prev = queue->rear;
	}
	queue->rear = now;
	queue->count++;
}

int Dequeue(Queue_t* queue)
{
	int ret = -1;
	Node_t* now;
	if (IsEmptyQ(queue))
		return ret;
	
	now = queue->front;
	ret = now->data;
	queue->front = now->next;
	//queue->front->prev = NULL;
	
	free(now);
	queue->count--;

	return ret;
}


void DeleteNode(Queue_t* queue, int data)
{
	Node_t* now = queue->front;
	Node_t* old;

	while (now != NULL && now->data != data) {
		old = now;
		now = now->next;
	}

	if (now == queue->front) {
		queue->front = now->next;
		//queue->front->prev = NULL;
	}
	else if (now == queue->rear) {
		//queue->rear = now->prev;
		queue->rear = old;
		queue->rear->next = NULL;
	}
	else {
		old->next = now->next;
		//now->prev->next = now->next;
		//now->next->prev = now->prev;
	}
	free(now);
}
