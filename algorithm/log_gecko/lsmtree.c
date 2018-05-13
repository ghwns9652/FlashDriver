#include "lsmtree.h"

level *level_init()
{
	level *res = (level *)malloc(sizeof(level) * MAX_LEVEL);
	for(int i = 0; i < MAX_LEVEL; i++)
	{
		res[i].indxes = i + 1;
		res[i].cnt = 0;
		//res[i].array = (node *)malloc(sizeof(node) * MAX_PAGE); ????
	}
	return res;
}

void level_free(level *lv)
{
	for(int i = 0; i < MAX_LEVEL; i++)
		free(lv[i].array);
	free(lv);
}

lsmtree *lsm_init()
{
	lsmtree *res = (lsmtree *)malloc(sizeof(lsmtree));
	res->levels = level_init();
	res->buffer = skiplist_init();
	return res;
}

void lsm_free(lsmtree *lsm)
{
	skiplist_free(lsm->buffer);
	level_free(lsm->levels);
	free(lsm);
}

void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag)
{
	skiplist_insert(lsm->buffer, key, offset, flag);
	if(lsm->buffer->size == MAX_PER_PAGE)
		skiplist_flush(lsm->buffer);
}