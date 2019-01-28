#ifndef HEAP_H
#define HEAP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/settings.h"
#include "Block.h"
#include "heap_node.h"

typedef struct Heap {
	int size;
	heap_node arr[_NOS+1];
}Heap;

void swap(Heap *heap, int a, int b);
void insert_heap(Heap *heap, Block *elem);
int delete_heap(Heap *heap);
void sift_down(Heap *heap, int parent, int last);
void construct_heap(Heap *heap);
#endif
