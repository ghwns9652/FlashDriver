#ifndef __LSM_HEADER__
#define __LSM_HEADER__

#include "skiplist.h"
#include "../../include/container.h"
#include "../../include/settings.h"
#include <math.h>

#define T_value 2
#define MAX_LEVEL (int)ceil(log((double)_NOB / (double)MAX_PER_PAGE) / log(T_value))
#define MAX_PAGE (int)ceil((double)_NOB / (double)MAX_PER_PAGE)

typedef struct node{
    KEYT max;
    KEYT min;
    PTR memptr;
}node;

typedef struct level{
    int indxes;
    int cnt;
    node **array;
}level;

typedef struct lsmtree{
	skiplist *buffer;
	level *levels;
}lsmtree;

level *level_init();
void level_free(level*);
lsmtree *lsm_init();
void lsm_free(lsmtree*);
void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag)

#endif
