#ifndef __GECKO_SKIPLIST_HEADER
#define __GECKO_SKIPLIST_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include "gecko_lsmtree.h"
#include "gecko_setting.h"

#define MAX_L 30 //max level number
#define PROB 4 //the probaility of level increasing : 1/PROB => 1/4

typedef struct snode{ //skiplist's node for Gecko Entry
	uint64_t VBM[4];
	KEYT key;
	ERASET erase;
	uint8_t level;
	struct snode **list;
}snode;

typedef struct skiplist{
	uint8_t level;
	uint64_t size;
	KEYT start;
	KEYT end;
	snode *header;
}skiplist;

//read only iterator. don't using iterater after delete iter's now node
typedef struct{
	skiplist *list;
	snode *now;
} sk_iter;

skiplist *skiplist_init(); //return initialized skiplist*
snode *skiplist_find(skiplist*, KEYT); //find snode having key in skiplist, return NULL:no snode
snode *skiplist_insert(skiplist*, KEYT, uint8_t, ERASET); //insert skiplist, return inserted snode
snode *skiplist_merge_insert(skiplist*, snode*); //insert skiplist, return inserted snode
int skiplist_delete(skiplist*, KEYT); //delete by key, return 0:normal -1:empty -2:no key
struct node* skiplist_flush(skiplist*); //
void skiplist_free(skiplist*);  //free skiplist
void skiplist_clear(skiplist*); //clear all snode in skiplist and  reinit skiplist
void skiplist_dump_key(skiplist*); //for test
void skiplist_dump_key_value(skiplist*); //for test
sk_iter* skiplist_get_iterator(skiplist*); //get read only iterator
snode *skiplist_get_next(sk_iter*); //get next snode by iterator

PTR skiplist_make_data(skiplist*);
#endif
