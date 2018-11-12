#include "block.h"

algo_req* assign_pseudo_req(uint8_t type, value_set *temp_v, request *req){
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	block_params *params = (block_params*)malloc(sizeof(block_params));
	pseudo_my_req->parents = req;
	params->type = type;
	params->value = temp_v;
	pseudo_my_req->end_req = block_end_req;
	pseudo_my_req->params = (void*)params;
	return pseudo_my_req;
}

/* Check Last offset */
int8_t block_CheckLastOffset(uint32_t LBA, uint32_t offset)
{
	if (BT[LBA].lastoffset > offset)
		return 0; // Bad case
	return 1; // Good case
}

value_set* SRAM_load(block_sram* sram_table, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	__block.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(GC_R, NULL, NULL)); // pull in gc is ALWAYS async
	sram_table[idx].SRAM_PTR = (PTR)malloc(PAGESIZE);
	sram_table[idx].SRAM_OOB = OOB[ppa];
	return temp_value_set;
}

void SRAM_unload(block_sram* sram_table, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(sram_table[idx].SRAM_PTR, FS_MALLOC_W, PAGESIZE);
	__block.li->push_data(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(GC_W, temp_value_set, NULL)); // ppa means new_PPA
	OOB[ppa] = sram_table[idx].SRAM_OOB;
	BM_ValidatePage(BM, ppa);
	free(sram_table[idx].SRAM_PTR);
}

#if 0
value_set* SRAM_load(block_sram* sram_table, uint32_t i, uint32_t old_PPA_zero, uint32_t sram_index)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	sram_table[sram_index].SRAM_PTR = (PTR)malloc(PAGESIZE);
	sram_table[sram_index].SRAM_OOB = OOB[old_PPA_zero + i];
	return temp_value_set;
	// sram malloc PAGESIZE 여기서. 인수로 sram 받아서 할당하고 unload에서 free.
}
void SRAM_unload(block_sram* sram_table, uint32_t i, uint32_t new_PPA_zero, uint32_t sram_index)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	//value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	value_set* temp_value_set2 = inf_get_valueset(sram_table[sram_index].SRAM_PTR, FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req2;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	OOB[new_PPA_zero + i] = sram_table[sram_index].SRAM_OOB;
	BM_ValidatePage(BM, new_PPA_zero+i);
	free(sram_table[sram_index].SRAM_PTR);
}
#endif


#if 0
value_set* SRAM_load(block_sram* sram_table, uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	sram_table[i].SRAM_PTR = (PTR)malloc(PAGESIZE);
	sram_table[i].SRAM_OOB = OOB[old_PPA_zero + i];
	return temp_value_set;
	// sram malloc PAGESIZE 여기서. 인수로 sram 받아서 할당하고 unload에서 free.
}
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

#if 0
void SRAM_unload(block_sram* sram_table, uint32_t i, uint32_t new_PPA_zero)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	//value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	value_set* temp_value_set2 = inf_get_valueset(sram_table[i].SRAM_PTR, FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	OOB[new_PPA_zero + i] = sram_table[i].SRAM_OOB;
	BM_ValidatePage(BM, new_PPA_zero+i);
	free(sram_table[i].SRAM_PTR);
}
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
	OOB[new_PPA_zero + i] = sram_table[i].SRAM_OOB;
	BM_ValidatePage(BM, new_PPA_zero+i);
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

