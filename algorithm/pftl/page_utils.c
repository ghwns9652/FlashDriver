#include "page.h"

algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req){
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	pbase_params *params = (pbase_params*)malloc(sizeof(pbase_params));
	pseudo_my_req->parents = req;
	params->type = type;
	params->value = temp_v;
	pseudo_my_req->end_req = pbase_end_req;
	pseudo_my_req->params = (void*)params;
	return pseudo_my_req;
}

value_set* SRAM_load(SRAM* sram, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	algo_pbase.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(GC_R, NULL, NULL)); // pull in gc is ALWAYS async
	sram[idx].PTR_RAM = (PTR)malloc(PAGESIZE);
    for(int i=0;i<ALGO_SEGNUM;i++){
	    sram[idx].OOB_RAM[i].lpa = page_OOB[ppa*ALGO_SEGNUM+i].lpa;
    }
	return temp_value_set;
}

void SRAM_unload(SRAM* sram, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset((PTR)sram[idx].PTR_RAM, FS_MALLOC_W, PAGESIZE);
	algo_pbase.li->push_data(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(GC_W, temp_value_set, NULL));
    for(int i=0;i<ALGO_SEGNUM;i++){
	    page_OOB[ppa*ALGO_SEGNUM + i].lpa = sram[idx].OOB_RAM[i].lpa;
    }
}

int32_t alloc_page(){
	static int32_t ppa = -1; // static for ppa
	Block *block;
	if((ppa != -1) && (ppa %( _g_ppb * ALGO_SEGNUM) == 0)){
		ppa = -1; // initialize that this need new block
	}
	if(ppa == -1){
		block = BM_Dequeue(free_b); // dequeue block from free block queue
		if(block){
			block->hn_ptr = BM_Heap_Insert(b_heap, block);
			ppa = block->PBA * _g_ppb * ALGO_SEGNUM;
		}
		else{
			ppa = pbase_garbage_collection();
		}
	}
	return ppa++;
}
