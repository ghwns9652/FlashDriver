#ifndef BLOCK_H
#define BLOCK_H

#include "heap_node.h"
struct heap_node;

typedef struct Block {
	struct heap_node *ptr;
	int cnt;
	int num;
}Block;

#endif
