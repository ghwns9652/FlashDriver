#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "normal.h"
#include "../../bench/bench.h"

extern MeasureTime mt;

struct algorithm __normal={
	.create=normal_create,
	.destroy=normal_destroy,
	.get=normal_get,
	.set=normal_set,
	.remove=normal_remove
};

typedef struct ppa_table{
	char valid;
	uint32_t lpa;
} ppa_node;

typedef struct lpa_table{
	uint32_t ppa;
	char valid;
} lpa_node;


static lpa_node *l_table;
static ppa_node *p_table;

uint32_t keyset(KEYT k){
	//non-occupied
	if (l_table[k].valid == 0) {
		l_table[k].valid = 1;
		l_table[k].ppa = rand() %200;
		p_table[l_table[k].ppa].lpa = k;
		p_table[l_table[k].ppa].valid = 1;
		return (uint32_t)l_table[k].ppa;
	}
	//pre-occupied
	else if (l_table[k].valid == 1) {
		
		KEYT tmp_ppa = l_table[k].ppa;

		while(p_table[tmp_ppa].valid == 1){
			tmp_ppa = rand() %211 - 11 ;
		}

		l_table[k].ppa = tmp_ppa;
		p_table[k].valid = 0;
		p_table[l_table[k].ppa].valid = 1;
		p_table[l_table[k].ppa].lpa = k;
		return (uint32_t)l_table[k].ppa;
	}
	return 0;
}

uint32_t keypull(KEYT k){
	if(l_table[k].valid == 1){
		return l_table[k].ppa;
	}
	return 0;
}

//

uint32_t normal_create (lower_info* li,algorithm *algo){
	algo->li=li;
	l_table = (lpa_node*)malloc(sizeof(lpa_node)*100);
	p_table = (ppa_node*)malloc(sizeof(ppa_node)*200);
	memset(l_table,0,sizeof(lpa_node)*100);
	memset(p_table,0,sizeof(ppa_node)*200);
	return 1;
}
void normal_destroy (lower_info* li, algorithm *algo){
	printf("\nsort by PPA\n");
	for(int i=0;i<200;i++)
		printf("ppa: %u valid:%d lpa:%u\n",i,(int)p_table[i].valid,p_table[i].lpa);
	printf("\nsort by LPA\n");
	for(int i=0;i<100;i++)
		printf("lpa: %u ppa:%u\n",i,l_table[i].ppa);

	return;
}
uint32_t normal_get(request *const req){
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	my_req->params=(void*)params;
	bench_algo_end(req);

	KEYT mapped_key = keypull(req->key);

	__normal.li->pull_data(mapped_key,PAGESIZE,req->value,req->isAsync,my_req);
	return 1;
}
uint32_t normal_set(request *const req){
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	my_req->params=(void*)params;
	bench_algo_end(req);
	
	KEYT mapped_key = keyset(req->key);

	__normal.li->push_data(mapped_key,PAGESIZE,req->value,req->isAsync,my_req);
	return 1;
}
uint32_t normal_remove(request *const req){
//	normal->li->trim_block()
	return 1;
}

void *normal_end_req(algo_req* input){
	normal_params* params=(normal_params*)input->params;
	
	request *res=input->parents;
	res->end_req(res);

	free(params);
	free(input);
	return NULL;
}
