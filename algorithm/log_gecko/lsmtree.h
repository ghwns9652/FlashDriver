#ifndef __LSM_HEADER__
#define __LSM_HEADER__

#include "../../include/container.h"
#include "../../include/settings.h"
#include <math.h>
#include <stdint.h>
#include "skiplist.h"

#define T_value 2
#define ERASET uint8_t

typedef struct node{
    KEYT max;
    KEYT min;
    PTR memptr;
    
}node;

typedef struct level{
    int indxes;
    int cur_cap;
    int max_cap;
    node **array;
}level;

typedef struct lsmtree{
	struct skiplist *buffer;
    struct skiplist *mixbuf;
	level *levels;
    int max_level;
}lsmtree;

level *level_init(int);
void level_free(level*, int);
lsmtree *lsm_init();
void lsm_free(lsmtree*);
void lsm_buf_update(lsmtree*, KEYT, uint8_t, ERASET);
void lsm_node_insert(lsmtree*, node*);

#endif
