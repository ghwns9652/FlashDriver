#include "lsmtree.h"

level *level_init(int max_lv)
{
	level *res = (level *)malloc(sizeof(level) * max_lv);
	for(int i = 0; i < max_lv; i++)
	{
		res[i].indxes = i + 1;
		res[i].cur_cap = 0;
		res[i].max_cap = pow(T_value, res[i].indxes);
		res[i].array = (node **)malloc(sizeof(node *) * res[i].max_cap);
	}
	return res;
}

void level_free(level *lv, int max_lv)
{
	for(int i = 0; i < max_lv; i++)
	{
		for(int j = 0; j < lv[i].max_cap; j++) // 문제 있음
			free(lv[i].array[j]);
		free(lv[i].array);
	}
	free(lv);
}

lsmtree *lsm_init()
{
	lsmtree *res = (lsmtree *)malloc(sizeof(lsmtree));
	res->max_level = (int)ceil(log((double)_NOB / (double)MAX_PER_PAGE) / log(T_value));
	res->levels = level_init(res->max_level);
	res->buffer = skiplist_init();
	return res;
}

void lsm_free(lsmtree *lsm)
{
	skiplist_free(lsm->buffer);
	level_free(lsm->levels, lsm->max_level);
	free(lsm);
}

void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag)
{
	skiplist_insert(lsm->buffer, key, offset, flag);
	if(lsm->buffer->size == MAX_PER_PAGE)
		lsm_node_insert(lsm, skiplist_flush(lsm->buffer));
}

void lsm_node_insert(lsmtree *lsm, node *data)
{
	lsm->levels[0].array[lsm->levels[0].cur_cap++] = data;
}