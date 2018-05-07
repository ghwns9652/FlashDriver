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

uint32_t block_create (lower_info*, algorithm *);
void block_destroy (lower_info*,  algorithm *);
uint32_t block_get(request *const);
uint32_t block_set(request *const);
//uint32_t block_get(request *);
//uint32_t block_set(request *);
uint32_t block_remove(request *const);
void* block_end_req(algo_req*);

void* block_algo_end_req(algo_req* input);
