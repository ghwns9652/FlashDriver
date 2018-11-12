#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include "../../include/container.h"
#include "../../interface/interface.h"
#include "../../bench/bench.h"
#include "../blockmanager/BM.h"

#if 0
typedef struct block_params{
	request *parents;
	int test;
	FSTYPE type;
}block_params;
#endif

#define DATA_R 0
#define DATA_W 1
#define GC_R 2
#define GC_W 3

/* global pointer/variable declaration */
//int32_t *block_maptable; // pointer to LPA->PPA table 
//int8_t *block_valid_array;
//uint32_t* lastoffset_array;
//uint32_t set_pointer = 0;
//value_set* sram_valueset;
//PTR* sram_value;

/* Table Structure Declaration */
typedef struct {
	int32_t PBA; // LBA to PBA
	uint32_t lastoffset; // last page offset
	int8_t valid; // valid block or not
	Block* alloc_block; // allocated block from Queue
	//uint8_t req_offset[ppb_]; // left or right request(in page). now, memory waste
} block_table;

typedef struct {
	Block segblock; // Block as a segment for using BM heap API
	Block** blockmap; 
} segment_table;
	
typedef struct {
	uint32_t LPA;
} block_OOB;

typedef struct {
	block_OOB SRAM_OOB;
	PTR SRAM_PTR;
} block_sram;

typedef struct {
	value_set *value;
	uint8_t type;
} block_params;

extern BM_T* BM;
extern b_queue* bQueue;
extern Heap* bHeap;
extern block_table* BT;
extern segment_table* ST;
extern segment_table* reservedSeg;
extern block_OOB* OOB;

extern algorithm __block;
extern uint32_t numLoaded;
extern int32_t nop_;
extern int32_t nob_;
extern int32_t nos_;
extern int32_t ppb_;
extern int32_t pps_;
extern int32_t bps_;

extern int numGC;

#if 0
inline int32_t LBA_TO_PSA(block_table* BT, uint32_t LBA) {
	return BT[LBA].PBA / bps_;
}
#endif
#define LBA_TO_PSA(BT, LBA) BT[LBA].PBA / bps_


/* Internal Function declaration */
value_set* SRAM_load(block_sram* sram_table, int32_t ppa, int idx);
void SRAM_unload(block_sram* sram_table, int32_t ppa, int idx);
void SRAM_unload_target(block_sram* sram_table, uint32_t i, uint32_t new_PPA_zero, algo_req* my_req);
algo_req* assign_pseudo_req(uint8_t type, value_set *temp_v, request *req);

void block_GC();
void GC_moving(request *const req, algo_req* my_req, uint32_t LBA, uint32_t offset, uint32_t old_PBA, uint32_t old_PPA);
int8_t block_CheckLastOffset(uint32_t LBA, uint32_t offset);

/* Macros */
#define VALID (1) // Block which has some filled pages(Invalid(EXIST) pages in BM)
#define ERASE (0) // Block which has empty pages(Valid(NONEXIST) pages in BM)
#define NIL (-1)
#define EXIST (1)
#define NONEXIST (0)

//#define BFTL_KEYDEBUG
#define BFTL_DEBUG0
//#define BFTL_DEBUG1
//#define BFTL_DEBUG2
//#define BFTL_DEBUG3
#define EPOCH (4095)

//#define Linear
#define Queue

uint32_t block_create (lower_info*, algorithm *);
void block_destroy (lower_info*,  algorithm *);
uint32_t block_get(request *const);
uint32_t block_set(request *const);
uint32_t block_remove(request *const);
void* block_end_req(algo_req*);
void* block_algo_end_req(algo_req* input);



void wb_full(void);
void __block_set(request *const req);
#define WB_SIZE 1024*1024
#define RPP 2	// Requests Per Page. Now, only support 2(8KB page = 4K+4K requests)
#endif // _BLOCK_H_
