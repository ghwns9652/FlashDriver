#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "../../include/container.h"
#include "../../interface/interface.h"
#include "../blockmanager/BM.h"

#include "block_queue.h"

//algo end req type definition.
#define TYPE uint8_t
#define DATA_R 0
#define DATA_W 1
#define GC_R 2
#define GC_W 3

typedef struct block_params{
	value_set *value;
	TYPE type;
}block_params;

/* global pointer/variable declaration */
extern int32_t nob_;
extern int32_t ppb_;
extern BM_T* BM;
extern int32_t *block_maptable; // pointer to LPA->PPA table
extern int8_t *block_valid_array;
extern uint32_t* lastoffset_array;
extern uint32_t set_pointer;
extern value_set* sram_valueset;

//block.c
uint32_t block_create(lower_info*, algorithm *);
void block_destroy(lower_info*,  algorithm *);
uint32_t block_get(request *const);
uint32_t block_set(request *const);
uint32_t block_remove(request *const);
void* block_end_req(algo_req*);

//block_utils.c
algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req);
value_set* SRAM_load(uint32_t i, uint32_t old_PPA_zero);
void SRAM_unload(uint32_t i, uint32_t new_PPA_zero);
int32_t block_findsp(int32_t checker);
int8_t block_CheckLastOffset(uint32_t* lastoffset_array, uint32_t PBA, uint32_t offset);

//block_gc.c
void GC_moving(request *const req, algo_req* my_req, uint32_t LBA, uint32_t offset, uint32_t PBA, uint32_t PPA, int8_t checker);

/* Macros */
#define VALID (1) // Block which has some filled pages(Invalid(EXIST) pages in BM)
#define ERASE (0) // Block which has empty pages(Valid(NONEXIST) pages in BM)
#define NIL (-1)
#define EXIST (1)
#define NONEXIST (0)

#endif
