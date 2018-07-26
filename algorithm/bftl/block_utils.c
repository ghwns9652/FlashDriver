#include "block.h"

algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req){
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
int8_t block_CheckLastOffset(block_status* bs, int32_t lba, int32_t offset){
	if (bs[lba].last_offset > offset){
		return 0;
	}
	return 1;
}

value_set* SRAM_load(block_sram* sram, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	__block.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(GC_R, NULL, NULL)); // pull in gc is ALWAYS async
	sram[idx].PTR_RAM = (PTR)malloc(PAGESIZE);
	sram[idx].OOB_RAM = block_oob[ppa];
	return temp_value_set;
}

void SRAM_unload(block_sram* sram, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(sram[idx].PTR_RAM, FS_MALLOC_W, PAGESIZE);
	__block.li->push_data(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(GC_W, temp_value_set, NULL));
	block_oob[ppa] = sram[idx].OOB_RAM;
	BM_ValidatePage(BM, ppa);
	free(sram[idx].PTR_RAM);
}
