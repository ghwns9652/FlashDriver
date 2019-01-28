#include "queue.h"

bool is_empty(Queue *queue) {
	if(size == 0) {
		return true;
	}
	return false;
}

void enqueue(Queue *queue, int num) {
	arr[rear] = num;
	rear = (rear + 1) % size;
	size++;
}

bool dequeue(Queue *queue) {
	if(is_empty) {
		return false;
	}

	front = (front + 1) % size;
	size--;
	return true;
}

int front() {
	return arr[front];
}
