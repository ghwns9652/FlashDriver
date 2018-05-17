#include "../../include/container.h"
#include "../../interface/interface.h"
#include "../../blockmanager/BM_Interface.h"
#include "../../bench/bench.h"
typedef struct block_params{
	request *parents;
	int test;
}block_params;

extern Block* blockArray;
extern Block** numValid_map;
extern Block** PE_map;

/* global pointer/variable declaration */
int32_t *block_maptable; // pointer to LPA->PPA table 
int8_t *block_valid_array;
uint32_t set_pointer = 0;

/* Macros */
#define VALID 1
#define ERASE 0
#define NIL -1
#define EXIST 1
#define NONEXIST 0

#define DMA_WRITE	1
#define DMA_READ	2

uint32_t block_create (lower_info*, algorithm *);
void block_destroy (lower_info*,  algorithm *);
uint32_t block_get(request *const);
uint32_t block_set(request *const);
uint32_t block_remove(request *const);
void* block_end_req(algo_req*);
void* block_algo_end_req(algo_req* input);
