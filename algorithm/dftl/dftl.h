#ifndef __H_DFTL__
#define __H_DFTL__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "../../interface/interface.h"
#include "../../interface/queue.h"
#include "../../include/container.h"
#include "../../include/dftl_settings.h"
#ifdef W_BUFF
#include "../lsmtree/skiplist.h"
#endif
#include "../blockmanager/BM.h"
#include "lru_list.h"

#define TYPE uint8_t
#define DATA_R 0
#define DATA_W 1
#define MAPPING_R 2
#define MAPPING_W 3
#define MAPPING_M 4
#define GC_MAPPING_W 5
#define TGC_R 6
#define TGC_W 7
#define DGC_R 8
#define DGC_W 9

#define DTIMESLOT 100
#define EPP (PAGESIZE / 4) //Number of table entries per page
#define D_IDX (lpa / EPP)	// Idx of directory table
#define P_IDX (lpa % EPP)	// Idx of page table

// Page table data structure
typedef struct demand_mapping_table{
	int32_t ppa; //Index = lpa
} D_TABLE;

// Cache mapping table data strcuture
typedef struct cached_table{
	int32_t t_ppa;
	int32_t idx;
	D_TABLE *p_table;
	NODE *queue_ptr;
	unsigned char flag; // 0: unchanged, 1: dirty, need to merge, 2: changed but all data on cache
} C_TABLE;

// OOB data structure
typedef struct demand_OOB{
	int32_t lpa;
} D_OOB;

// SRAM data structure (used to hold pages temporarily when GC)
typedef struct demand_SRAM{
	int32_t origin_ppa;
	D_OOB OOB_RAM;
	D_TABLE *DATA_RAM;
} D_SRAM;

typedef struct demand_params{
	value_set *value;
	pthread_mutex_t dftl_mutex;
	TYPE type;
} demand_params;

typedef struct read_params{
	int32_t t_ppa;
	uint8_t read;
} read_params;

typedef struct mem_table{
	D_TABLE *mem_p;
} mem_table;

typedef struct dftltime{
	uint64_t dftl_cdf[4][1000000/DTIMESLOT+1];
} dftl_time;

typedef struct page_buffer{
	unsigned char status; // 0: empty, 1: half-written, 2: fully-written
	KEYT ppa;
	PTR value;
} page_buffer;

typedef struct page_buf_header{
	uint32_t idx;
	page_buffer *buffer;
} page_buf_header;

/* extern variables */
extern algorithm __demand;

extern b_queue *free_b;
extern Heap *data_b;
extern Heap *trans_b;

extern C_TABLE *CMT; // Cached Mapping Table
extern uint8_t *VBM;
extern b_queue *mem_q;
extern D_OOB *demand_OOB; // Page level OOB

extern BM_T *bm;
extern Block *t_reserved;
extern Block *d_reserved;

extern int32_t trans_gc_poll;
extern int32_t data_gc_poll;

extern int32_t num_page;
extern int32_t num_block;
extern int32_t p_p_b;
extern int32_t num_tpage;
extern int32_t num_tblock;
extern int32_t num_dpage;
extern int32_t num_dblock;
extern int32_t max_cache_entry;
extern int32_t num_max_cache;

extern int32_t tgc_count;
extern int32_t dgc_count;
extern int32_t read_tgc_count;
extern int32_t tgc_w_dgc_count;

extern page_buf_header* pbuf_header;
/* extern variables */

//dftl.c
uint32_t demand_create(lower_info*, algorithm*);
void demand_destroy(lower_info*, algorithm*);
void *demand_end_req(algo_req*);
uint32_t demand_set(request *const);
uint32_t demand_get(request *const);
uint32_t __demand_set(request *const);
uint32_t __demand_get(request *const);
uint32_t demand_remove(request *const);
uint32_t demand_eviction(char, bool*);

//dftl_utils.c
algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req);
D_TABLE* mem_deq(b_queue *q);
void mem_enq(b_queue *q, D_TABLE *input);
void merge_w_origin(D_TABLE *src, D_TABLE *dst);
int lpa_compare(const void *a, const void *b);
int32_t tp_alloc(char, bool*);
int32_t dp_alloc();
value_set* SRAM_load(D_SRAM* d_sram, int32_t ppa, int idx, char t);
void SRAM_unload(D_SRAM* d_sram, int32_t ppa, int idx, char t);
void buffer_push_data(KEYT PPA, uint32_t size, value_set* value, bool async, algo_req *const req);
void buffer_pull_data(KEYT PPA, uint32_t size, value_set* value, bool async, algo_req *const req);
void cache_show(char* dest);

//garbage_collection.c
int32_t tpage_GC();
int32_t dpage_GC();

#endif
