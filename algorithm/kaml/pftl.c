#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "pftl.h"
#include "heap.h"
#include "Queue.h"
#include "gc.h"
#include "../../bench/bench.h"

#define LOWERTYPE 10

extern MeasureTime mt;
struct algorithm algo_pftl = {
	.create = pftl_create,
	.destroy = pftl_destroy,
	.read = pftl_read,
	.write = pftl_write,
	.remove = pftl_remove
};

n_cdf _cdf[LOWERTYPE];

char temp[PAGESIZE];

bool is_full;
int log_seg_num = _NOS - 1;	// reserve segment number
uint8_t garbage_table[_NOP/8];	// valid bitmap
int OOB[_NOP];
int mapping_table[_NOP];	// use LPA as index, use 1 block as log block
int gc_read_cnt;
int gc_target_cnt;
Block garbage_cnt[_NOS];	// count garbage pages in each block(=segment)
Block trash;
Heap heap;
Queue ppa_queue;

void pftl_cdf_print() {
}
uint32_t pftl_create(lower_info *li,algorithm *algo) {
	algo->li=li;
	heap.size = 0;
	ppa_queue.size = 0;
	ppa_queue.front = 0;
	ppa_queue.rear = 0;
	memset(temp,'x',PAGESIZE);
	memset(mapping_table, -1, sizeof(mapping_table));
	memset(garbage_table, 0, sizeof(garbage_table));
	memset(OOB, -1, sizeof(OOB));
	for(int i = 0; i < LOWERTYPE; i++) {
		_cdf[i].min = UINT_MAX;
	}
	for(int i = 0; i < _NOS; i++) {
		garbage_cnt[i].num = -1;
	}
	for(int i = 0; i < _NOP - _PPS; i++) {
		enqueue(&ppa_queue, i);
	}
	heap.arr[0].block = &trash;
	heap.arr[0].block->cnt = -1;
	heap.arr[0].block->num = -1;
	printf("_NOP: %ld\n", _NOP);
	printf("_NOS: %ld\n", _NOS);

	return 1;
}
void pftl_destroy(lower_info* li, algorithm *algo) {
	pftl_cdf_print();
	return;
}

uint32_t ppa = 0;
int g_cnt;
static int reserv_ppa_start = (_NOP - _PPS);
int erase_seg_num;


uint32_t pftl_read(request *const req) {
//	printf("read key: %d\n", req->key);
	bench_algo_start(req);
	normal_params* params = (normal_params*)malloc(sizeof(normal_params));
	params->test = -1;

	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = pftl_end_req;
	my_req->params = (void*)params;
	my_req->type = DATAR;
	my_req->ppa = mapping_table[req->key];

	bench_algo_end(req);
	algo_pftl.li->read(my_req->ppa, PAGESIZE, req->value, req->isAsync, my_req);
	return 1;
}
uint32_t pftl_write(request *const req) {
	bench_algo_start(req);
	normal_params* params = (normal_params*)malloc(sizeof(normal_params));
	params->test = -1;

	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = pftl_end_req;
	bench_algo_end(req);
	my_req->type = DATAW;
	my_req->params = (void*)params;
	

	if(!is_full) {		// First write on ppa
		ppa = front(&ppa_queue);
		bool dequeue_res = dequeue(&ppa_queue);

		// overwrite
		if(mapping_table[req->key] != -1) {
			garbage_table[mapping_table[req->key]/8] |= (1<<(mapping_table[req->key] % 8)); // 1: invalid 0: valid
			garbage_cnt[mapping_table[req->key]/_PPS].cnt++;	//garbage count in block
			OOB[mapping_table[req->key]] = -1;
		}
		else {
			garbage_table[ppa/8] &= ~(1 << (ppa % 8));
		}
		mapping_table[req->key] = ppa;
		OOB[ppa] = req->key;	// update unused OOB

		if(garbage_cnt[ppa/_PPS].num == -1) {
			garbage_cnt[ppa/_PPS].num = ppa/_PPS;
			garbage_cnt[ppa/_PPS].cnt = 0;
			insert_heap(&heap, &garbage_cnt[ppa/_PPS]);
		}

		if((is_empty(&ppa_queue)) && (!is_full)) {	// page over
			is_full = true;
		}

		my_req->ppa = mapping_table[req->key];
		algo_pftl.li->write(my_req->ppa, PAGESIZE, req->value, req->isAsync, my_req);
	}
	else if(is_full) {	//if queue is not empty
		construct_heap(&heap);		// sort(find segment number that has biggest invalid count)
		int max = delete_heap(&heap);  // max is segment number to erase
		reserv_ppa_start = garbage_collection(log_seg_num*_PPS, max);
		int reserv_ppa_end = ((reserv_ppa_start / _PPS) + 1) * _PPS;

		for(int i = reserv_ppa_start; i < reserv_ppa_end; i++) {
			enqueue(&ppa_queue, i);
		}
		is_full = false;
	}


	return 0;
}
uint32_t pftl_remove(request *const req) {
	algo_pftl.li->trim_block(req->key, NULL);
//	table[req->key] = -1;	//dead data -> -1, set it's dead data
	return 1;
}
void *pftl_end_req(algo_req* input) {
	normal_params *params = (normal_params*)input->params;
	request *res = input->parents;
	value_set* value;
	
	switch(input->type){
		case DATAR: 
		case DATAW:
			if(res){
//				printf("[END REQ] end_req\n");
				res->end_req(res);
//				printf("[END REQ] end_req_end\n");
			}
			free(params);
			break;
		case GCDR: 
			gc_target_cnt++;
			break;
		case GCDW: 
		//	printf("[PFTL_END_REQ]end request GC_W\n");
			//printf("[PFTL_END_REQ]res->value: %x\n", res->value);
		//	printf("[PFTL_END_REQ]res->value->value: %s\n", res->value->value);
			value = (value_set *)input->params;
			inf_free_valueset(value, FS_MALLOC_W);
			break;
	}
	free(input);
	
	return NULL;
}
