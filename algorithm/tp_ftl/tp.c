#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "tp.h"
#include "../../bench/bench.h"

#define LOWERTYPE 10

extern MeasureTime mt;
struct algorithm algo_tpbase={
	.create=tp_create,
	.destroy=tp_destroy,
	.get=tp_get,
	.set=tp_set,
	.remove=tp_remove
};

uint32_t tp_create (lower_info* li,algorithm *algo){
	algo->li=li;

	return 1;
}
void tp_destroy (lower_info* li, algorithm *algo){
	return;
}

uint32_t tp_get(request *const req){
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=tp_end_req;
	my_req->params=(void*)params;
	my_req->type=DATAR;

	bench_algo_end(req);
	algo_tpbase.li->pull_data(req->key,PAGESIZE,req->value,req->isAsync,my_req);
	return 1;
}
uint32_t tp_set(request *const req){
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=tp_end_req;
	bench_algo_end(req);
	my_req->type=DATAW;
	my_req->params=(void*)params;
	
	algo_tpbase.li->push_data(req->key,PAGESIZE,req->value,req->isAsync,my_req);
	return 0;
}
uint32_t tp_remove(request *const req){
	algo_tpbase.li->trim_block(req->key,NULL);
	return 1;
}
void *tp_end_req(algo_req* input){
	normal_params* params=(normal_params*)input->params;
	request *res=input->parents;
	
	//Deliver end_req to interface layer
	res->end_req(res);
	free(params);
	free(input);
	return NULL;
}
