#include "heap.h"

void swap(Heap *heap, int a, int b) {
	Block *b1 = heap->arr[a].block;
	Block *b2 = heap->arr[b].block;
	heap_node tmp = heap->arr[a];
	heap->arr[a] = heap->arr[b];
	heap->arr[b] = tmp;
	b1->ptr = &heap->arr[b];
	b2->ptr = &heap->arr[a];
}

void insert_heap(Heap *heap, Block *elem) {
	int i = ++(heap->size);
	heap->arr[i].block = elem;
	elem->ptr = &(heap->arr[i]);
	printf("[AT INSERT] heap size: %d\n", heap->size);
	printf("[AT INSERT] elem num: %d\n", elem->num);
	printf("[AT INSERT] elem cnt: %d\n", elem->cnt);

	while(i >= 1) {
		if((heap->arr[i].block->cnt > heap->arr[i/2].block->cnt) && (heap->arr[i/2].block->cnt != -1)) {
			swap(heap, i, i/2);
			i /= 2;
			continue;
		}
		break;
	}
}

int delete_heap(Heap *heap) {
	int parent = 1;
	int child = 2;
	int max;
	int ret = heap->arr[1].block->num;
	swap(heap, 1, heap->size);
	heap->arr[heap->size].block->ptr = NULL;
	heap->arr[heap->size].block = NULL;
	heap->size--;
	
	while(child <= heap->size) {
		if((child + 1) <= heap->size) {
			if(heap->arr[child].block->cnt > heap->arr[child+1].block->cnt) {
				max = child;
			}
			else {
				max = child + 1;
			}
		}
		else {
			max = child;		
		}
		// bigger than all child -> break
		if(heap->arr[parent].block->cnt > heap->arr[max].block->cnt) {
			break;
		}
		swap(heap, parent, max);
		parent = max;
		child *= 2;
	}

	return ret;
}

void sift_down(Heap *heap, int parent, int last) {
	int left, right, max;
	
	while((parent*2) <= last) {
		left = parent*2;
		right = parent*2 + 1;
		max = parent;

		if(heap->arr[left].block->cnt > heap->arr[max].block->cnt) {
			max = left;
		}
		if(right <= last && (heap->arr[right].block->cnt > heap->arr[max].block->cnt)) {
			max = right;
		}

		if(parent != max) {
			swap(heap, parent, max);
			parent = max;
		}
		else {
			return;
		}
	}
}

void construct_heap(Heap *heap) {
	int parent = (heap->size / 2);

	while(parent > 0) {
		sift_down(heap, parent--, heap->size);
	}
}
