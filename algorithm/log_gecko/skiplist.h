#ifndef __SKIPLIST_HEADER
#define __SKIPLIST_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

#define MAX_L 30 //max level number
#define PROB 4 //the probaility of level increasing : 1/PROB => 1/4
#define KEYT uint32_t //key type
#define VALUE 64
#define ERASET uint8_t //erase type

//typedef enum {false, true} bool;
/* 수정 */
typedef struct snode{ //skiplist's node
	KEYT key;
	uint8_t level;
	uint64_t VBM[4];
	ERASET erase;
	struct snode **list;
}snode;

typedef struct skiplist{
	uint8_t level;
	uint64_t size;
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
int skiplist_delete(skiplist*, KEYT); //delete by key, return 0:normal -1:empty -2:no key
void skiplist_free(skiplist *list);  //free skiplist
void skiplist_clear(skiplist *list); //clear all snode in skiplist and  reinit skiplist
void skiplist_dump_key(skiplist * list); //for test
void skiplist_dump_key_value(skiplist * list); //for test
sk_iter* skiplist_get_iterator(skiplist *list); //get read only iterator
snode *skiplist_get_next(sk_iter* iter); //get next snode by iterator
#endif
