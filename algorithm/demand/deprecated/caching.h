#ifndef __CACHING_H__
#define __CACHING_H__

#include <stdio.h>
#include <stdlib.h>
#include "../../include/setting.h"
#include "../../include/dftl_setting.h"
#include "../../include/container.h"

#include "lru.h"

#define NUM_ENTRY (PAGESIZE / 4)
#define NUM_T_PAGE (TOTALSIZE / (NUM_ENTRY * PAGEISZE))
#define T_LPN (lpn / NUM_ENTRY)     
#define P_OFFSET (lpn % NUM_ENTRY)

#define CACHE_LOAD_SUCCESS 0
#define CACHE_LOAD_FAIL    1

//Cache variables
struct cached_mapping_table *cmt;
struct global_table_directory *gtd;
struct t_page *t_page;
extern LRU *lru_list;
extern LRU *entry_list;



/* Structures */

//Cached Mapping Table (CMT)
struct cached_mapping_table{
	int32_t cache_total_size;
	int32_t cache_free_size;
	struct t_page **c_page;
};

//Global Table Directory (GTD)
struct global_table_directory{
	int32_t m_ppn;			        // Physical translation page location
	bool state;				// Clean = 0, Dirty = 1
};
//Translation Page
struct t_page{

	int32_t *ppn;              		// Physical_location	
	struct s_node *s_field;	   		// For S-FTL data structure
	struct tp_node *tp_field;  		// For TPFTL data structure
	void *lru_pointer;         		// translation pointer in LRU
	void *lru_entry_pointer;		// Entry pointer in LRU
};

struct s_node{
	int32_t *head_entires;     
	void *bitmap;
};
struct tp_node{
	int32_t lpn;
	int32_t ppn;
	char    count;
};

/* Functions */
void cache_init(struct cached_mapping_table *, struct t_page *);
void cache_free(struct cached_mapping_table *, struct t_page *);
int32_t cache_load(int32_t,int32_t,char);
int32_t cache_unload(int32_t,int32_t,char);

#endif
