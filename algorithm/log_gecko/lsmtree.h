#ifndef __LSM_HEADER__
#define __LSM_HEADER__

#include "../../include/container.h"
#include "../../include/settings.h"
#include <math.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include "skiplist.h"
#include "lsmsetting.h"

typedef struct node{
    KEYT max;
    KEYT min;
    PTR memptr;
    uint32_t offset;
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
    int fd;
}lsmtree;

level *level_init(int);
void level_free(level*, int);
lsmtree *lsm_init();
void lsm_free(lsmtree*);
void lsm_buf_update(lsmtree*, KEYT, uint8_t, ERASET);
void lsm_node_insert(lsmtree*, node*);
void lsm_node_fwrite(lsmtree*, int, int);
void lsm_node_recover(lsmtree*, int, int);

#endif
