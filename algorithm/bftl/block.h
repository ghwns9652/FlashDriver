#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "../../include/container.h"
#include "../../interface/interface.h"
#include "../blockmanager/BM.h"
#include "../../bench/bench.h"

#include "block_queue.h"

typedef struct block_params{
	request *parents;
	int test;
	FSTYPE type;
}block_params;

/* global pointer/variable declaration */
int32_t *block_maptable; // pointer to LPA->PPA table 
int8_t *block_valid_array;
uint32_t* lastoffset_array;
uint32_t set_pointer = 0;
value_set* sram_valueset;

/* Internal Function declaration */
value_set* SRAM_load(uint32_t i, uint32_t old_PPA_zero);
void SRAM_unload(uint32_t i, uint32_t new_PPA_zero);
void GC_moving(request *const req, algo_req* my_req, uint32_t LBA, uint32_t offset, uint32_t PBA, uint32_t PPA, int8_t checker);
int32_t block_findsp(int32_t checker);
int8_t block_CheckLastOffset(uint32_t* lastoffset_array, uint32_t PBA, uint32_t offset);

/* Macros */
#define VALID (1) // Block which has some filled pages(Invalid(EXIST) pages in BM)
#define ERASE (0) // Block which has empty pages(Valid(NONEXIST) pages in BM)
#define NIL (-1)
#define EXIST (1)
#define NONEXIST (0)

//#define BFTL_KEYDEBUG
//#define BFTL_DEBUG1
//#define BFTL_DEBUG2
//#define BFTL_DEBUG3
#define EPOCH (4095)

#define Linear
//#define Queue

uint32_t block_create (lower_info*, algorithm *);
void block_destroy (lower_info*,  algorithm *);
uint32_t block_get(request *const);
uint32_t block_set(request *const);
uint32_t block_remove(request *const);
void* block_end_req(algo_req*);
void* block_algo_end_req(algo_req* input);
