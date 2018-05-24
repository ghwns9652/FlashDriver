#ifndef __GECKO_LSM_HEADER__
#define __GECKO_LSM_HEADER__

#include <math.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include "gecko_skiplist.h"
#include "gecko_setting.h"

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
    struct skiplist *mixbuf;
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

#endif
