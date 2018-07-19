#include "page.h"
#include "../../bench/bench.h"

struct algorithm algo_pbase={
	.create = pbase_create,
	.destroy = pbase_destroy,
	.get = pbase_get,
	.set = pbase_set,
	.remove = pbase_remove
};

b_queue *free_b;
Heap *b_heap;

TABLE *page_TABLE;
P_OOB *page_OOB;
uint8_t *VBM;

Block *block_array;
Block *reserved;
int32_t gc_load;

int32_t num_page;
int32_t num_block;
int32_t p_p_b;
int32_t gc_count;

uint32_t pbase_create(lower_info* li, algorithm *algo){
	/*
	   init table and oob area.
	   set global as macro.
	   init blockmanager and heap.
	*/
	num_page = _NOP;
	num_block = _NOS;
	p_p_b = _PPS;
	gc_count = 0;

	//printf("number of block: %d\n", num_block);
	//printf("page per block: %d\n", p_p_b);
	//printf("number of page: %d\n", num_page);

	page_TABLE = (TABLE*)malloc(sizeof(TABLE) * num_page);
	page_OOB = (P_OOB*)malloc(sizeof(P_OOB) * num_page);
	VBM = (uint8_t*)malloc(num_page);
	algo->li = li;

	for(int i = 0; i < num_page; i++){
     	page_TABLE[i].ppa = -1;
		page_OOB[i].lpa = -1;
	}

	memset(VBM, 0, num_page);

	BM_Init(&block_array);
	reserved = &block_array[0];

	BM_Queue_Init(&free_b);
	for(int i = 1; i < num_block; i++){
		BM_Enqueue(free_b, &block_array[i]);
	}
	b_heap = BM_Heap_Init(num_block - 1);
	return 0;
}

void pbase_destroy(lower_info* li, algorithm *algo){
	/*
	   abandon all memories.
	   destroy blockmanager.
	*/
	printf("gc count: %d\n", gc_count);
	BM_Queue_Free(free_b);
	BM_Heap_Free(b_heap);
	BM_Free(block_array);
	free(VBM);
	free(page_OOB);
	free(page_TABLE);
}

void *pbase_end_req(algo_req* input){
	/*
	   end params and algo_request.
	   differs accroding to types.
	*/
	pbase_params *params = (pbase_params*)input->params;
	value_set *temp_v = params->value;
	request *res = input->parents;

	switch(params->type){
		case DATA_R:
			if(res){
				res->end_req(res);
			}
			break;
		case DATA_W:
			if(res){
				res->end_req(res);
			}
			break;
		case GC_R://custom_read from SRAM_load.
			gc_load++;	
			break;
		case GC_W://custom write from SRAM_unload.
			inf_free_valueset(temp_v, FS_MALLOC_W);
			break;
	}
	free(params);
	free(input);
	return NULL;
}

uint32_t pbase_get(request* const req){
	/*
	   sends pull req to lower.
	   req made from assign_pseudo_req.
	   find ppa from table.
	*/
	int32_t lpa;
	int32_t ppa;

	bench_algo_start(req);
	lpa = req->key;
	ppa = page_TABLE[lpa].ppa;
	if(ppa == -1){
		bench_algo_end(req);
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}
	bench_algo_end(req);	
	algo_pbase.li->pull_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));
	return 0;
}

uint32_t pbase_set(request* const req){
	/*
	   sends push req to lower.
	   !!pushes data before mapping.!!
	   maps table + updates table.
	*/
	int32_t lpa;
	int32_t ppa;
	bench_algo_start(req);
	lpa = req->key;
	ppa = alloc_page();
	bench_algo_end(req);
	algo_pbase.li->push_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req));
	if(page_TABLE[lpa].ppa != -1){
		VBM[page_TABLE[lpa].ppa] = 0;
		block_array[page_TABLE[lpa].ppa/p_p_b].Invalid++;
	}
	page_TABLE[lpa].ppa = ppa;
	VBM[ppa] = 1;
	page_OOB[ppa].lpa = lpa;
	return 0;
}

uint32_t pbase_remove(request* const req){
	int32_t lpa;
	bench_algo_start(req);
	lpa = req->key;
	page_TABLE[lpa].ppa = -1; //reset to default.
	page_OOB[lpa].lpa = -1; //reset reverse_table to default.
	bench_algo_end(req);
	return 0;
}
