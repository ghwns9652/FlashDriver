#include "block.h"

value_set* SRAM_load(block_sram* sram_table, uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	sram_table[i].SRAM_PTR = (PTR)malloc(PAGESIZE);
	//sram_table[i].SRAM_OOB = OOB[old_PPA_zero + i];
	return temp_value_set;
	// sram malloc PAGESIZE 여기서. 인수로 sram 받아서 할당하고 unload에서 free.
}
#if 0
value_set* SRAM_load(PTR* sram_value, uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	sram_value[i] = (PTR)malloc(PAGESIZE);
	return temp_value_set;
	// sram malloc PAGESIZE 여기서. 인수로 sram 받아서 할당하고 unload에서 free.
}
#endif

void SRAM_unload(block_sram* sram_table, uint32_t i, uint32_t new_PPA_zero)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	//value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	value_set* temp_value_set2 = inf_get_valueset(sram_table[i].SRAM_PTR, FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	//OOB[new_PPA_zero + i] = sram_table[i].SRAM_OOB;
	free(sram_table[i].SRAM_PTR);
}
#if 0
void SRAM_unload(PTR* sram_value, uint32_t i, uint32_t new_PPA_zero)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	//value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	value_set* temp_value_set2 = inf_get_valueset(sram_value[i], FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	free(sram_value[i]);
}
#endif
void SRAM_unload_target(block_sram* sram_table, uint32_t i, uint32_t new_PPA_zero, algo_req* my_req)
{
	value_set* temp_value_set2 = inf_get_valueset(sram_table[i].SRAM_PTR, FS_MALLOC_W, PAGESIZE);
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, my_req);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	free(sram_table[i].SRAM_PTR);
}
#if 0
void SRAM_unload_target(PTR* sram_value, uint32_t i, uint32_t new_PPA_zero, algo_req* my_req)
{
	value_set* temp_value_set2 = inf_get_valueset(sram_value[i], FS_MALLOC_W, PAGESIZE);
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, my_req);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	free(sram_value[i]);
}
#endif

