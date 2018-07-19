#ifndef __GECKO__HEADER
#define __GECKO__HEADER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include "gecko_setting.h"

extern int32_t num_page;
extern int32_t num_block;
extern int32_t p_p_b;
extern int32_t gepp;

typedef struct snode{ //skiplist's node for Gecko Entry
	BITMAP VBM[2048];
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

typedef struct node{
    KEYT max;
    KEYT min;
    uint32_t LPA;
    PTR memptr;
}node;

typedef struct level{
    int cur_cap;
    int max_cap;
    node *array;
}level;

typedef struct lsmtree{
	struct skiplist *buffer;
    struct skiplist *compaction;
	level *levels;
    int max_level;
    int fd;
}lsmtree;

level *level_init(int);
void level_free(level*, int);
lsmtree *lsm_init();
void lsm_free(lsmtree*);
void lsm_buf_update(lsmtree*, KEYT, uint8_t, ERASET);
void lsm_buffer_flush(lsmtree*, node*);
void lsm_node_fwrite(lsmtree*, int, int);
void lsm_node_recover(lsmtree*, int, int);
int lsm_merge(lsmtree*, int);
void print_level_status(lsmtree*);

skiplist *skiplist_init(); //return initialized skiplist*
snode *skiplist_find(skiplist*, KEYT); //find snode having key in skiplist, return NULL:no snode
snode *skiplist_insert(skiplist*, KEYT, uint32_t, ERASET); //insert skiplist, return inserted snode
snode *skiplist_snode_insert(skiplist*, snode*); //insert skiplist, return inserted snode
int skiplist_delete(skiplist*, KEYT); //delete by key, return 0:normal -1:empty -2:no key
struct node* skiplist_flush(skiplist*);
void skiplist_free(skiplist*);  //free skiplist
void skiplist_clear(skiplist*); //clear all snode in skiplist and  reinit skiplist
void skiplist_dump_key(skiplist*); //for test
void skiplist_dump_key_value(skiplist*); //for test
sk_iter* skiplist_get_iterator(skiplist*); //get read only iterator
sk_iter *skiplist_get_iter_from_here(skiplist *, snode*);
snode *skiplist_get_next(sk_iter*); //get next snode by iterator

PTR skiplist_make_data(skiplist*);
PTR skiplist_lsm_merge(skiplist*, KEYT, KEYT*, KEYT*);
#endif
