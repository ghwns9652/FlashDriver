#include "Queue.h"

bool is_empty(Queue *queue) {
	if(queue->size == 0) {
		return true;
	}
	return false;
}

void enqueue(Queue *queue, int num) {
	queue->size++;
	queue->arr[queue->rear] = num;
	queue->rear = (queue->rear + 1) % (_NOP);
}

bool dequeue(Queue *queue) {
	if(is_empty(queue)) {
		return false;
	}

	queue->front = (queue->front + 1) % (_NOP);
	queue->size--;
	return true;
}

int front(Queue *queue) {
	return queue->arr[queue->front];
}
