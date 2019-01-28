#include "heap.h"

void swap(invalid_cnt *a, invalid_cnt *b) {
	invalid_cnt temp = *a;
	*a = *b;
	*b = temp;
}

void insert_heap(Heap *heap, invalid_cnt elem) {
	int i = ++(heap->size);
	heap->arr[i].cnt = elem.cnt;
	heap->arr[i].block_num = elem.block_num;

	while(i >= 1) {
		if((heap->arr[i].cnt > heap->arr[i/2].cnt) && (heap->arr[i/2].cnt != -1)) {
			swap(&heap->arr[i], &heap->arr[i/2]);
			i /= 2;
			continue;
		}
		break;
	}
}

int delete_heap(Heap *heap) {
	swap(&heap->arr[1], &heap->arr[heap->size]);
	int parent = 1;
	int child = 2;
	int max;
	heap->size--;
	
	while(child <= heap->size) {
		if(heap->arr[child].cnt > heap->arr[child+1].cnt) {
			max = child;
		}
		else {
			max = child + 1;
		}
		// bigger than all child -> break
		if(heap->arr[parent].cnt > heap->arr[max].cnt) {
			break;
		}
		swap(&heap->arr[parent], &heap->arr[max]);
		parent = max;
		child *= 2;
	}

	return heap->arr[1].block_num;
}

void sift_down(Heap *heap, int parent, int last) {
	int left, right, max;
	
	while((parent*2) <= last) {
		left = parent*2;
		right = parent*2 + 1;
		max = parent;

		if(heap->arr[left].cnt > heap->arr[max].cnt) {
			max = left;
		}
		if(right <= last && (heap->arr[right].cnt > heap->arr[max].cnt)) {
			max = right;
		}

		if(parent != max) {
			swap(&heap->arr[parent], &heap->arr[max]);
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
