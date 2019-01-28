#include <stdbool.h>

// TODO: initialize size, front, rear
typedef struct Queue {
	int size;
	int front;
	int rear;
	int arr[_NOP];
}Queue;

bool is_empty(Queue *queue);
void enqueue(Queue *queue, int num);
bool dequeue(Queue *queue);
int front();
