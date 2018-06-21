#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../../include/container.h"
#include "../../interface/interface.h"
#include "../../interface/queue.h"
#include "dftl_queue.h"
/*
k:
define 된 변수들을 하나의 파일로 따로 빼서 관리할것.
../../include/dftl_settings.h
 */

#define TYPE uint8_t
#define DATA_R 0
#define DATA_W 1
#define MAPPING_R 2
#define MAPPING_W 3
#define GC_R 4
#define GC_W 5

/* Data page unit DFTL header */
#ifdef UNIT_D
#define CACHESIZE (32*K)
#define EPP (PAGESIZE / (int)sizeof(D_TABLE)) //Number of table entries per page
#define NTP (_NOP / EPP) //Number of Translation Page
#define	GTDSIZE ((int)sizeof(D_TABLE) * NTP)
#define GTDENT (GTDSIZE/(int)sizeof(D_TABLE))	// Num of GTD entries
#define CMTSIZE ((int)sizeof(C_TABLE) * ((CACHESIZE - GTDSIZE) / (int)sizeof(C_TABLE)))
#define CMTENT (CMTSIZE/(int)sizeof(C_TABLE))	// Num of CMT entries
#define D_IDX (lpa/EPP)	// Idx of directory table
#define P_IDX (lpa%EPP)	// Idx of page table

// Page table data structure
typedef struct demand_mapping_table{
	int32_t ppa; //Index = lpa ->k: uint32_t
}D_TABLE;

// Cache mapping table data strcuture
typedef struct cached_table{
	int32_t lpa; // k:KEYT ex)-1==UINT_MAX(header - limits.h)
	int32_t ppa; // k:KEYT
	unsigned char flag; // 0: unchanged, 1: changed
	LINKED_LIST *queue_ptr;
}C_TABLE;

// OOB data structure
typedef struct demand_OOB{
	int32_t reverse_table;
	unsigned char valid_checker; // 0: invalid, 1: valid -> k:따로 빼시오,bit로 바꿔
}D_OOB;

// SRAM data structure (used to hold pages temporarily when GC)
typedef struct demand_SRAM{
	D_OOB OOB_RAM;
	value_set *valueset_RAM;
}D_SRAM;

typedef struct demand_params{
	int test;
	TYPE type;
	pthread_mutex_t *lock;
}demand_params;

uint32_t demand_create(lower_info*, algorithm*);
void demand_destroy(lower_info*, algorithm*);
uint32_t demand_get(request *const);
uint32_t demand_set(request *const);
uint32_t demand_remove(request *const);
void *demand_end_req(algo_req*);
void *pseudo_end_req(algo_req*);
algo_req* assign_pseudo_req(TYPE type);
int CMT_check(int32_t lpa, int32_t *ppa);
uint32_t demand_eviction(int *CMT_i);
char btype_check();
void tpage_GC();
void dpage_GC();
void SRAM_load(int32_t ppa, int idx);
void SRAM_unload(int32_t ppa, int idx);
int lpa_compare(const void *a, const void *b);
int ppa_compare(const void *a, const void *b);
bool demand_GC(char btype);
void dp_alloc(int32_t *ppa);
void tp_alloc(int32_t *t_ppa);
#endif

// use more space to gain performance improvement or use less space while costs some cache performance
// Consider!!
/* Translation page unit DFTL */
#ifdef UNIT_T
#define CACHESIZE (32*K)
#define EPP (PAGESIZE / (int)sizeof(D_TABLE)) //Number of table entries per page
#define NTP (_NOP / EPP) //Number of Translation Page
#define CMTSIZE ((int)sizeof(C_TABLE) * NTP)
#define CMTENT NTP // Num of CMT entries
#define D_IDX (lpa/EPP)	// Idx of directory table
#define P_IDX (lpa%EPP)	// Idx of page table
#define MAXTPAGENUM 4 // max number of tpage on ram // Must be changed according to cache size

// Page table data structure
typedef struct demand_mapping_table{
	int32_t ppa; //Index = lpa
}D_TABLE;

// Cache mapping table data strcuture
typedef struct cached_table{
	int32_t t_ppa;
	D_TABLE *p_table;	
	unsigned char flag; // 0: unchanged, 1: changed
	LINKED_LIST *queue_ptr;
}C_TABLE;

// OOB data structure
typedef struct demand_OOB{
	int32_t reverse_table;
	unsigned char valid_checker; // 0: invalid, 1: valid
}D_OOB;

// SRAM data structure (used to hold pages temporarily when GC)
typedef struct demand_SRAM{
	D_OOB OOB_RAM;
	value_set *valueset_RAM;
}D_SRAM;

typedef struct demand_params{
	int test;
	TYPE type;
	value_set *value;
}demand_params;

uint32_t demand_create(lower_info*, algorithm*);
void demand_destroy(lower_info*, algorithm*);
uint32_t demand_get(request *const);
uint32_t __demand_get(request *const);
uint32_t demand_set(request *const);
uint32_t __demand_set(request *const);
uint32_t demand_remove(request *const);
void *demand_end_req(algo_req*);
void *pseudo_end_req(algo_req*);
algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req);
D_TABLE* CMT_check(int32_t lpa, int32_t *ppa);
uint32_t demand_eviction();
char btype_check();
void tpage_GC();
void dpage_GC();
void SRAM_load(int32_t ppa, int idx);
void SRAM_unload(int32_t ppa, int idx);
int lpa_compare(const void *a, const void *b);
int ppa_compare(const void *a, const void *b);
bool demand_GC(char btype);
void dp_alloc(int32_t *ppa);
void tp_alloc(int32_t *t_ppa);
void cache_show(char* dest);
#endif
