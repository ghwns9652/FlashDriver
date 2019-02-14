#ifndef __LRU_H__
#define __LRU_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct __node{
	void *data;           //translation page pointer (Page unit) for DFTL, SFTL
	struct __node *next;
	struct __node *prev;
}NODE;


typedef struct __lru{
	int size;
	NODE *head;
	NODE *tail;
}LRU;

void lru_init(LRU**);
void lru_free(LRU*);
NODE* lru_push(LRU*, void*);
void* lru_pop(LRU*);
void lru_update(LRU*, NODE*);
void lru_delete(LRU*, NODE*);

#endif
