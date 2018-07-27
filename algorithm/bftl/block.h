#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "../../include/container.h"
#include "../../interface/interface.h"
#include "../blockmanager/BM.h"

//algo end req type definition.
#define TYPE uint8_t
#define DATA_R 0
#define DATA_W 1
#define GC_R 2
#define GC_W 3

typedef struct block_status{
	int32_t pba;
	int32_t last_offset;
	Block* alloc_block;
} block_status;

typedef struct seg_status{
	Block seg;
	Block **block;
} seg_status;

typedef struct block_OOB{
	int32_t lpa;
} B_OOB;

typedef struct block_sram{
	B_OOB OOB_RAM;
	PTR PTR_RAM;
} block_sram;

typedef struct block_params{
	value_set *value;
	TYPE type;
} block_params;

/* global pointer/variable declaration */
extern algorithm __block;
extern int32_t nop_;
extern int32_t nob_;
extern int32_t ppb_;
extern int32_t nos_;
extern int32_t pps_;
extern int32_t bps_;
extern int32_t numLoaded;
extern BM_T* BM;
extern b_queue *free_b;
extern Heap *b_heap;
extern seg_status *SS;
extern seg_status *reserved;
extern block_status* BS;
extern B_OOB* block_oob;

//block.c
uint32_t block_create(lower_info*, algorithm *);
void block_destroy(lower_info*,  algorithm *);
uint32_t block_get(request *const);
uint32_t block_set(request *const);
uint32_t block_remove(request *const);
void* block_end_req(algo_req*);

//block_utils.c
algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req);
int8_t block_CheckLastOffset(int32_t lba, int32_t offset);
value_set* SRAM_load(block_sram* sram, int32_t ppa, int idx);
void SRAM_unload(block_sram* sram, int32_t ppa, int idx);

//block_gc.c
void GC_moving(value_set *data, int32_t lba, int32_t offset);
void block_GC();

#endif
