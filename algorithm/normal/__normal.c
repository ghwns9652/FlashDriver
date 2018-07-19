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

typedef struct ppa_node{
	char valid;
	uint32_t lpa;
} ppa_node;

typedef struct lpa_node{
	uint32_t ppa;
	char valid;
	char *value;
} lpa_node;


static lpa_node *l_table;
static ppa_node *p_table;

void setkey(request *const req ,KEYT k){
	//non-occupied
	if (l_table[k].valid == 0) {
		l_table[k].valid = 1;
		l_table[k].ppa = rand() %200;
		p_table[l_table[k].ppa].lpa = k;
		p_table[l_table[k].ppa].valid = 1;

		req->key = l_table[k].ppa;
		return;
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

		req->key = l_table[k].ppa;
		return;
	}
	return;
}

void getkey(request * const req,KEYT k){
	if(l_table[k].valid == 1){
		req->key = l_table[k].ppa;		
		return;
	}
	return;
}

void setvalue(request * const req, KEYT k){
	char* tmp_value = (char*)malloc(sizeof(char)*PAGESIZE);
	memset(tmp_value,(char)k,PAGESIZE);
	req->value->value = tmp_value;
}

uint32_t normal_create (lower_info* li,algorithm *algo){
	algo->li=li;
	l_table = (lpa_node*)malloc(sizeof(lpa_node)*100);
	p_table = (ppa_node*)malloc(sizeof(ppa_node)*200);
	memset(l_table,0,sizeof(lpa_node)*100);
	memset(p_table,0,sizeof(ppa_node)*200);
	return 1;
}
void normal_destroy (lower_info* li, algorithm *algo){
	printf("\n-------sort by PPA-------\n");
	for(int i=0;i<200;i++){
		printf(" ppa: %u valid:%c lpa:%u \n",i,p_table[i].valid,p_table[i].lpa);
	}
	printf("\n--------sort by LPA-------\n");
	for(int i=0;i<100;i++){
		char *tmp = l_table[i].value;
		printf(" lpa: %u ppa:%u value: %d \n",i,l_table[i].ppa,(int)tmp[0]);
//	printf("\n-------------------------\n");
	}
	return;
}

uint32_t normal_get(request *const req){
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=9;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	my_req->params=(void*)params;
	bench_algo_end(req);

	getkey(req,req->key); //ppa -> lpa

	__normal.li->pull_data(req->key,PAGESIZE,req->value,req->isAsync,my_req);
	return 1;
}
uint32_t normal_set(request *const req){
	bench_algo_start(req);
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));
	params->test=5;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	my_req->params=(void*)params;
	bench_algo_end(req);

	setvalue(req,req->key);
	setkey(req,req->key); //lpa -> ppa

	__normal.li->push_data(req->key,PAGESIZE,req->value,req->isAsync,my_req);
	return 1;
}
uint32_t normal_remove(request *const req){
//	normal->li->trim_block()
	return 1;
}

void *normal_end_req(algo_req* input){
	normal_params* params=(normal_params*)input->params;

	//set
	if(params->test == 5) {
		char *tmp_value  = input->parents->value->value;
		uint32_t tmp_lpa = input->parents->key;
		l_table[tmp_lpa].value = tmp_value;
	}
//	//get
//	else if(params->test == 9) { 
//		char *tmp_value  = input->parents->value->value;
//		uint32_t tmp_lpa = p_table[input->parents->key].lpa;
//		l_table[tmp_lpa].value = tmp_value;
//	}
	
	request *res=input->parents;
	res->end_req(res);

	free(params);
	free(input);
	return NULL;
}
