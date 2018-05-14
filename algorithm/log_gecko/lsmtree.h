#ifndef __LSM_HEADER__
#define __LSM_HEADER__

#include "skiplist.h"
#include "../../include/container.h"
#include "../../include/settings.h"
#include <math.h>

#define T_value 2

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
	skiplist *buffer;
	level *levels;
    int max_level;
}lsmtree;

level *level_init(int);
void level_free(level*, int);
lsmtree *lsm_init();
void lsm_free(lsmtree*);
void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag);
void lsm_node_insert(lsmtree *lsm, node *data);

#endif
