#define _LARGEFILE64_SOURCE
#include "gecko_lsmtree.h"

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
	res->fd = open("./storage.data", O_RDWR|O_CREAT|O_TRUNC, 0666);
	if(res->fd == -1)
	{
		printf("file open error!\n");
		exit(-1);
	}
	return res;
}

void lsm_free(lsmtree *lsm)
{
	skiplist_free(lsm->buffer);
	level_free(lsm->levels, lsm->max_level);
	close(lsm->fd);
	free(lsm);
}

void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag)
{
	//printf("key %d\n", key);
	skiplist_insert(lsm->buffer, key, offset, flag);
	if(lsm->buffer->size == MAX_PER_PAGE)
		lsm_buffer_flush(lsm, skiplist_flush(lsm->buffer));
}

void lsm_node_fwrite(lsmtree *lsm, int lv_off, int nd_off)
{
	int off_cal = 0;
	for(int i = 0; i < lv_off; i++)
		off_cal += lsm->levels[i].max_cap;
	off_cal += nd_off;
	lseek64(lsm->fd, (off64_t)(PAGESIZE * off_cal), SEEK_SET);
	write(lsm->fd, lsm->levels[lv_off].array[nd_off]->memptr, PAGESIZE);
	free(lsm->levels[lv_off].array[nd_off]->memptr);
}

void lsm_buffer_flush(lsmtree *lsm, node *data)
{
	if(lsm->levels[0].cur_cap == lsm->levels[0].max_cap)
		lsm_merge(lsm, 0);
	lsm->levels[0].array[lsm->levels[0].cur_cap] = data;
	lsm_node_fwrite(lsm, 0, lsm->levels[0].cur_cap++);
	lsm->buffer = skiplist_init();
}

void lsm_node_recover(lsmtree *lsm, int lv_off, int nd_off)
{
	int off_cal = 0, loc = 0;
	PTR temp = (PTR)malloc(PAGESIZE);
	lsm->mixbuf = skiplist_init();
	for(int i = 0; i < lv_off; i++)
		off_cal += lsm->levels[i].max_cap;
	off_cal += nd_off;
	lseek64(lsm->fd, (off64_t)(PAGESIZE * off_cal), SEEK_SET);
	read(lsm->fd, temp, PAGESIZE);
	for(int i = 0; i < MAX_PER_PAGE; i++)
	{
		skiplist_merge_insert(lsm->mixbuf, (snode*)&temp[loc]);
		loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
	}
	free(temp);
	//skiplist_dump_key_value(lsm->mixbuf);
	skiplist_free(lsm->mixbuf);
}

void lsm_merge(lsmtree *lsm, int lv_off)
{
	int off_cal = 0, loc = 0;
	PTR temp = (PTR)malloc(PAGESIZE);
	lsm->mixbuf = skiplist_init();
	for(int i = 0; i < lv_off; i++)
		off_cal += lsm->levels[i].max_cap;
	lseek64(lsm->fd, (off64_t)(PAGESIZE * off_cal), SEEK_SET);
	read(lsm->fd, temp, PAGESIZE);
	for(int i = 0; i < MAX_PER_PAGE; i++)
	{
		skiplist_merge_insert(lsm->mixbuf, (snode*)&temp[loc]);
		loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
	}
	free(temp);
	//skiplist_dump_key_value(lsm->mixbuf);
	skiplist_free(lsm->mixbuf);
}
