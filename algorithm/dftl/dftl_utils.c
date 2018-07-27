#include "dftl.h"

algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req){
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	demand_params *params = (demand_params*)malloc(sizeof(demand_params));
	pseudo_my_req->parents = req;
	params->type = type;
	params->value = temp_v;
#if EVICT_POLL
	if(type == MAPPING_M || type == MAPPING_W){
#else
	if(type == MAPPING_M){
#endif
		pthread_mutex_init(&params->dftl_mutex, NULL);
		pthread_mutex_lock(&params->dftl_mutex);
	}
	pseudo_my_req->end_req = demand_end_req;
	pseudo_my_req->params = (void*)params;
	return pseudo_my_req;
}

D_TABLE* mem_deq(b_queue *q){ // inefficient function for give heap pointer
	return (D_TABLE*)dequeue(q);
}

void mem_enq(b_queue *q, D_TABLE *input){ // inefficient function for free heap pointer
	enqueue(q, (void*)input);
}

void merge_w_origin(D_TABLE *src, D_TABLE *dst){ // merge trans table.
	for(int i = 0; i < EPP; i++){
		if(dst[i].ppa == -1){
			dst[i].ppa = src[i].ppa;
		}
		else if(src[i].ppa != -1){
			BM_InvalidatePage(bm, src[i].ppa);
		}
	}
}

int lpa_compare(const void *a, const void *b){
	uint32_t num1 = (uint32_t)(((D_SRAM*)a)->OOB_RAM.lpa);
	uint32_t num2 = (uint32_t)(((D_SRAM*)b)->OOB_RAM.lpa);
	if(num1 < num2){
		return -1;
	}
	else if(num1 == num2){
		return 0;
	}
	else{
		return 1;
	}
}

int32_t tp_alloc(char req_t, bool *flag){
	static int32_t ppa = -1; // static for ppa
	Block *block;
	if(ppa != -1 && ppa % p_p_b == 0){
		ppa = -1; // initialize that this need new block
	}
	if(ppa == -1){
		if(trans_b->idx == trans_b->max_size){ // to maintain heap struct
			ppa = tpage_GC();
			if(req_t == 'R'){
				read_tgc_count++;
			}
			else if(req_t == 'D'){
				tgc_w_dgc_count++;
			}
			if(flag){
				*flag = true;
			}
			return ppa++;
		}
		block = BM_Dequeue(free_b); // dequeue block from free block queue
		if(block){
			block->hn_ptr = BM_Heap_Insert(trans_b, block);
			block->type = 1; // 1 is translation block
			ppa = block->PBA * p_p_b;
		}
		else{
			ppa = tpage_GC();
			if(req_t == 'R'){
				read_tgc_count++;
			}
			else if(req_t == 'D'){
				tgc_w_dgc_count++;
			}
			if(flag){
				*flag = true;
			}
		}
	}
	return ppa++;
}

int32_t dp_alloc(){ // Data page allocation
	static int32_t ppa = -1; // static for ppa
	Block *block;
	if(ppa != -1 && ppa % p_p_b == 0){
		ppa = -1; // initialize that this need new block
	}
	if(ppa == -1){
		if(data_b->idx == data_b->max_size){ // to maintain heap struct
			ppa = dpage_GC();
			return ppa++;
		}
		block = BM_Dequeue(free_b); // dequeue block from free block queue
		if(block){
			block->hn_ptr = BM_Heap_Insert(data_b, block);
			block->type = 2; // 2 is data block
			ppa = block->PBA * p_p_b;
		}
		else{
			ppa = dpage_GC();
		}
	}
	return ppa++;
}

value_set* SRAM_load(D_SRAM* d_sram, int32_t ppa, int idx, char t){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	if(t == 'T'){
		__demand.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(TGC_R, NULL, NULL));
	}
	else{
#if P_BUFF
		buffer_pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(DGC_R, NULL, NULL));
#else
		__demand.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(DGC_R, NULL, NULL));
#endif
	}
	d_sram[idx].DATA_RAM = (D_TABLE*)malloc(PAGESIZE);
	d_sram[idx].OOB_RAM = demand_OOB[ppa];
	d_sram[idx].origin_ppa = ppa;
	return temp_value_set;
}

void SRAM_unload(D_SRAM* d_sram, int32_t ppa, int idx, char t){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset((PTR)d_sram[idx].DATA_RAM, FS_MALLOC_W, PAGESIZE);
	if(t == 'T'){
		__demand.li->push_data(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(TGC_W, temp_value_set, NULL));
	}
	else{
#if P_BUFF
		buffer_pull_data(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(DGC_W, temp_value_set, NULL));
#else
		__demand.li->push_data(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(DGC_W, temp_value_set, NULL));
#endif
	}
	demand_OOB[ppa] = d_sram[idx].OOB_RAM;
	BM_ValidatePage(bm, ppa);
	free(d_sram[idx].DATA_RAM);
}

void buffer_push_data(KEYT PPA, uint32_t size, value_set* value, bool async, algo_req *const req){
	uint32_t *idx = &(pbuf_header->idx);
	page_buffer *buffer = pbuf_header->buffer;
	algo_req *my_req;
	value_set *temp_set;

	buffer = buffer + *idx;
	switch(buffer->status){
		case 0: // buffer empty
			buffer->ppa = PPA/2;
			memcpy(buffer->value, value->value, PAGESIZE/2);
			buffer->status = 1;
			req->end_req(req);
			break;
		case 1: // buffer half full
			if(buffer->ppa != PPA/2){
				printf("page_buffer ppa error!\n");
			}
			memcpy((buffer->value) + PAGESIZE/2, (value->value) + PAGESIZE/2, PAGESIZE/2);
			buffer->status = 2;
			*idx = *idx + 1;
			req->end_req(req);
			break;
		case 2: // buffer full
			printf("page_buffer idx error!\n");
			break;
	}
	if((*idx) == PB_SIZE){
		for(int i = 0; i < PB_SIZE; i++){
			if(buffer[i].status != 2){
				printf("page_buffer error!\n");
			}
			temp_set = inf_get_valueset(buffer[i].value, FS_MALLOC_W, PAGESIZE);
			my_req = assign_pseudo_req(DATA_W, temp_set, NULL);
			__demand.li->push_data(buffer[i].ppa, PAGESIZE, temp_set, async, my_req);
			inf_free_valueset(temp_set, FS_MALLOC_W);
			buffer[i].status = 0;
			buffer[i].ppa = UINT32_MAX;
		}
		*idx = 0;
	}
	return ;
}

void buffer_pull_data(KEYT PPA, uint32_t size, value_set* value, bool async, algo_req *const req){
	page_buffer *buffer = pbuf_header->buffer;
	uint32_t i = 0;

	while(buffer[i].status != 0){
		if(buffer[i].ppa == PPA/2){
			memcpy(value->value, buffer[i].value, PAGESIZE);
			req->end_req(req);
			return ;
		}
		i++;
	}
	__demand.li->pull_data(PPA, size, value, async, req);
	return ;
}

/* Print page_table that exist in d_idx */
void cache_show(char* dest){
	int parse = 16;
	for(int i = 0; i < EPP; i++){
		printf("%d ", ((D_TABLE*)dest)[i].ppa);
		if((i % parse) == parse - 1){
			printf("\n");
		}
	}
}
