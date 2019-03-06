#include <stdio.h>
#include <stdlib.h>
#include "caching.h"


void cache_init(struct cache_mapping_table *cmt, struct t_page * t_page)
{
	//CMT allocation
	cmt = (struct cache_mapping_table *)malloc(sizeof(struct cache_mapping_table));
	cmt->cache_total_size = NUM_T_PAGE * PAGESIZE;
	cmt->cache_free_size = cache_total_size;
	cmt->c_page = (struct t_page **)malloc(sizeof(struct t_page *) * NUM_T_PAGE);
	
	//GTD allocation
	gtd = (struct global_table_directory *)malloc(sizeof(struct global_table_directory) * NUM_T_PAGE);
	
	//Physical Translation page allocation
	t_page = (struct t_page *)malloc(sizeof(struct t_page) * NUM_T_PAGE);

	//Cache init
	for(int i = 0; i < NUM_T_PAGE; i++)
	{
		cmt->c_page[i] = NULL;
		gtd[i].m_ppn = -1;
		gtd[i].state = 0;
		t_page[i].ppn = (int32_t *)malloc(sizeof(int32_t) * NUM_ENTRY);
	
		memset(t_page[i].ppn, -1, sizeof(int32_t) * NUM_ENTRY);	

	}

	lru_init(&lru_list);
	//lru_init(&entry_list); //For TPFTL

}

void cache_free(struct cache_mapping_table *cmt)
{
	free(cmt);
	free(gtd);
	free(t_page);
	free(lru_list);
}

int32_t cache_load(int32_t trans_lpn, int32_t ppn, char type)
{
	struct t_page *t_page;
	int32_t *p_table;
	int32_t m_ppn;	

	m_ppn = gtd[trans_lpn].m_ppn;
	t_page = t_page[m_ppn];

	if(cmt->c_page[trans_lpn] != NULL)
	{
		lru_update(lru_list, (void *)cmt->c_page[trans_lpn]->lru_pointer);
		return 1;
	}

	//Load translation page
	if(cmt->cache_free_size >= PAGESIZE)
	{
		cmt->c_page[trans_lpn] = t_page;	//Caching translation page
		if(type == 'W')
			gtd[trans_lpn].state = 1;
		else
			gtd[trasn_lpn].state = 0;

		cmt->c_page[trans_lpn]->lru_pointer = lru_push(lru_list, (void *)t_page);

	}
	//Unload translation page
	else
	{
		

	}


	return CACHE_LOAD_SUCCESS;
}
