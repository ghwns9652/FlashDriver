#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/settings.h"
#include "invalid_cnt.h"

typedef struct Heap {
	int size;
	invalid_cnt arr[_NOS];
}Heap;

void swap(invalid_cnt *a, invalid_cnt *b);
void insert_heap(Heap *heap, int num);
int delete_heap(Heap *heap);
void sift_down(Heap *heap, int parent, int last);
void construct_heap(Heap *heap);
