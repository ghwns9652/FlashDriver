#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "pftl.h"
#include "heap.h"
#include "Queue.h"
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
uint8_t garbage_table[_NOP/8];	// sequentially save garbage PPA
int mapping_table[_NOP];	// use LPA as index, use 1 block as log block
Block garbage_cnt[_NOS];	// count garbage pages in each block(=segment)
Block trash;
Heap heap;
Queue queue;
int *point_heap[_NOS];	// point to each heap element

void pftl_cdf_print() {
}
uint32_t pftl_create(lower_info *li,algorithm *algo) {
	algo->li=li;
	heap.size = 0;
	queue.size = 0;
	queue.front = 0;
	queue.rear = 0;
	memset(temp,'x',PAGESIZE);
	memset(mapping_table, -1, sizeof(mapping_table));
	for(int i = 0; i < LOWERTYPE; i++) {
		_cdf[i].min = UINT_MAX;
	}
	for(int i = 0; i < _NOS; i++) {
		garbage_cnt[i].num = -1;
	}
	for(int i = 0; i < _NOP; i++) {
		enqueue(&queue, i);
	}
	heap.arr[0].block = &trash;
	heap.arr[0].block->cnt = -1;
	heap.arr[0].block->num = -1;
	printf("_NOP: %ld\n", _NOP);
	printf("_NOS: %ld\n", _NOS);
	printf("_PPS: %ld\n", _PPS);

	return 1;
}
void pftl_destroy(lower_info* li, algorithm *algo) {
	pftl_cdf_print();
	return;
}

uint32_t ppa = 0;
int g_cnt = 0;
uint8_t logb_no = 63;		// log block's number
int seg_num;
int page_num;

uint32_t pftl_read(request *const req) {
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

	// initial write
	if(!is_full) {
		// overwrite
		if(mapping_table[req->key] != -1) {
			seg_num = (mapping_table[req->key] / _PPS);
			page_num = (mapping_table[req->key] % _PPS);
			garbage_table[seg_num] |= (1 << page_num);
			garbage_cnt[seg_num].cnt++;	//garbage count in block
		}
		
		int next = front(&queue);
		mapping_table[req->key] = next;
		dequeue(&queue);
		if(garbage_cnt[next/_PPS].num == -1) {
			garbage_cnt[next/_PPS].num = next/_PPS;
			garbage_cnt[next/_PPS].cnt = 0;
			insert_heap(&heap, &garbage_cnt[next/_PPS]);
		}

		if((is_empty(&queue)) && (!is_full)) {	// page over
			is_full = true;
		}
	}
	else if(is_full) {	//After GC()
		construct_heap(&heap);
		int max = delete_heap(&heap);
		printf("[AT PFTL] max: %d\n", max);
	}

	my_req->ppa = mapping_table[req->key];
	algo_pftl.li->write(my_req->ppa, PAGESIZE, req->value, req->isAsync, my_req);

	return 0;
}
uint32_t pftl_remove(request *const req) {
//	algo_pftl.li->trim_block(req->key, NULL);
	mapping_table[req->key] = -1;	//garbage data -> -1
	return 1;
}
void *pftl_end_req(algo_req* input) {
	normal_params* params = (normal_params*)input->params;
	//bool check=false;
	//int cnt=0;
	request *res = input->parents;
	res->end_req(res);

	free(params);
	free(input);
	return NULL;
}
