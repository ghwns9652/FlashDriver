#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "pftl.h"
#include "heap.h"
#include "../../bench/bench.h"

#define LOWERTYPE 10

extern MeasureTime mt;
struct algorithm algo_pftl={
	.create=pftl_create,
	.destroy=pftl_destroy,
	.read=pftl_read,
	.write=pftl_write,
	.remove=pftl_remove
};

n_cdf _cdf[LOWERTYPE];

char temp[PAGESIZE];

bool is_full;
int table[_NOP];	// use lpa as index, use 1 block as log block
int garbage[_NOP];	// sequentially save garbage ppa
uint16_t garbage_cnt[_NOS];	// count garbage pages in each block (block == segment)

void pftl_cdf_print(){
	int a = 0;
	for(int i = 0; i < _NOP; i++){
		if(garbage[i] != -1){
			a++;
		}
	}
}
uint32_t pftl_create(lower_info *li,algorithm *algo){
	algo->li=li;
	memset(temp,'x',PAGESIZE);
	memset(table, -1, sizeof(table));
	memset(garbage, -1, sizeof(garbage));
	for(int i=0; i<LOWERTYPE; i++){
		_cdf[i].min=UINT_MAX;
	}
	printf("_NOP: %ld\n", _NOP);
	printf("_NOB: %ld\n", _NOB);

	return 1;
}
void pftl_destroy(lower_info* li, algorithm *algo){
	pftl_cdf_print();
	return;
}

uint32_t ppa;
int g_cnt;
uint8_t logb_no=63;		// log block's number
uint32_t pftl_read(request *const req){		// read, key: lba
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=pftl_end_req;
	my_req->params=(void*)params;
	my_req->type=DATAR;
	my_req->ppa=table[req->key];

	bench_algo_end(req);
	algo_pftl.li->read(my_req->ppa,PAGESIZE,req->value,req->isAsync,my_req);
	return 1;
}
uint32_t pftl_write(request *const req){		// write
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->type=DATAW;
	my_req->params=(void*)params;
	my_req->end_req=pftl_end_req;

	if(is_full){
		table[req->key] = garbage[g_cnt];
	}
	else{
		if(table[req->key] != -1){	// overwrite
			garbage[(g_cnt++)%_NOP] = table[req->key];
			garbage_cnt[ppa/_PPB]++;
		}
		table[req->key] = ppa++;
	}
	if((ppa == _NOP) && (!is_full)){
		is_full = true;
	}
	my_req->ppa = table[req->key];

	bench_algo_end(req);
	algo_pftl.li->write(my_req->ppa,PAGESIZE,req->value,req->isAsync,my_req);
	return 0;
}
uint32_t pftl_remove(request *const req){
	algo_pftl.li->trim_block(req->key,NULL);
//	table[req->key] = -1;	//dead data -> -1, set it's dead data
	return 1;
}
void *pftl_end_req(algo_req* input){
	normal_params* params=(normal_params*)input->params;
	//bool check=false;
	//int cnt=0;
	request *res=input->parents;
	res->end_req(res);

	free(params);
	free(input);
	return NULL;
}
