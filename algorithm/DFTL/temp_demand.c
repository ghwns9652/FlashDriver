#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "demand.h"
#include "../../bench/bench.h"

struct algorithm __demand={
	.create = demand_create,
	.destroy = demand_destroy,
	.get = demand_get,
	.set = demand_set,
	.remove = demand_remove
};

extern LINKED_LIST* head;
extern LINKED_LIST* tail;

C_TABLE *CMT;
D_TABLE *GTD;
D_OOB *demand_OOB;
D_SRAM *d_sram;

int32_t DPA_status;
int32_t TPA_status;
int32_t PBA_status;
int8_t needGC;

uint32_t demand_create(lower_info* li, algorithm* algo){
	/* Table, OOB, sram allocation */
	CMT = (C_TABLE*)malloc(CMTSIZE);
	GTD = (D_TABLE*)malloc(GTDSIZE);
	demand_OOB = (D_OOB*)malloc(sizeof(D_OOB) * _NOP);
	d_sram = (D_SRAM*)malloc(sizeof(D_SRAM) * _PPB);

	algo->li = li; // Insert lower info

	/* Initialization */
	for(int i = 0; i < CMTENT; i++){
		CMT[i] = (C_TABLE){-1, -1, 0, NULL};
}
	for(int i = 0; i < GTDENT; i++){
		GTD[i].ppa = -1;
	}
	for(int i = 0; i < _NOP; i++){
		demand_OOB[i] = (D_OOB){-1, 0};
	}
	for(int i = 0; i < _PPB; i++){
		d_sram[i] = (D_SRAM){-1, NULL};
	}
	DPA_status = 0;
	TPA_status = 0;
	PBA_status = 0;
	needGC = 0;
}

void demand_destroy(lower_info* li, algorithm* algo){
	free(CMT);
	free(GTD);
	free(demand_OOB);
	free(d_sram);
	while(head)
		queue_delete(head);
}

uint32_t demand_get(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	int CMT_i;
	D_TABLE *p_table;
	
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;
	
	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_update(CMT[CMT_i].queue_ptr);
		bench_algo_end(req);
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
	}
	else{
		demand_eviction(&CMT_i);
		t_ppa = GTD[D_IDX].ppa;
		p_table = (D_TABLE*)malloc(PAGESIZE);
		__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0);
		ppa = p_table[P_IDX].ppa;
		free(p_table);
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 0, queue_insert((void*)(CMT + CMT_i))};
		bench_algo_end(req);
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
	}
}

uint32_t demand_set(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int CMT_i;

	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){ // CACHE HIT
		demand_OOB[ppa].valid_checker = 0;
		dp_alloc(&ppa);
		CMT[CMT_i].ppa = ppa;
		CMT[CMT_i].flag = 1;
		queue_update(CMT[CMT_i].queue_ptr);
		demand_OOB[ppa] = (D_OOB){lpa, 1};
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
	}
	else{
		demand_eviction(&CMT_i);
		dp_alloc(&ppa);
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 1, queue_insert((void*)(CMT + CMT_i))};
		demand_OOB[ppa] = (D_OOB){lpa, 1};
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
	}
}

uint32_t demand_remove(request *const req){
}

void *demand_end_req(algo_req* input){
	request *res = input->parents;
	res->end_req(res);
	free(input);
}

void *pseudo_end_req(algo_req* input){
	free(input);
}

algo_req* assign_pseudo_req(){
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	pseudo_my_req->parents = NULL;
	pseudo_my_req->end_req = pseudo_end_req;

	return pseudo_my_req;
}

int CMT_check(int32_t lpa, int32_t *ppa){
	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].lpa == lpa){
			*ppa = CMT[i].ppa;
			return i;
		}
	}
	return -1;
}

uint32_t demand_eviction(int *CMT_i){
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	D_TABLE *p_table;
	algo_req *pseudo_my_req;

	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].lpa == -1){
			*CMT_i = i;
			return 0;
		}
	}
	*CMT_i = (int)((C_TABLE*)(tail->DATA) - CMT);
	lpa = CMT[*CMT_i].lpa;
	ppa = CMT[*CMT_i].ppa;
	if(CMT[*CMT_i].flag != 0){
		p_table = (D_TABLE*)malloc(PAGESIZE);
		if((t_ppa = GTD[D_IDX].ppa) != -1){
			__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0);
			demand_OOB[t_ppa].valid_checker = 0;
		}
		p_table[P_IDX].ppa = ppa;
		tp_alloc(&t_ppa);
		__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0);
		demand_OOB[t_ppa] = (D_OOB){D_IDX, 1};
		GTD[D_IDX].ppa = t_ppa;
		free(p_table);
	}
	queue_delete(tail);
	CMT[*CMT_i] = (C_TABLE){-1, -1, 0, NULL};
}

void dp_alloc(int32_t *ppa){
	if(DPA_status % _PPB == 0){
		if(PBA_status == _NOB)
			needGC = 1;
		if(needGC == 1){
			//while(!demand_GC(PBA_status, 'D'))
				//PBA_status++;
		}
		else
			DPA_status = PBA_status * _PPB;
		PBA_status++;
	}
	*ppa = DPA_status;
	DPA_status++;
}

void tp_alloc(int32_t *t_ppa){
	if(TPA_status % _PPB == 0){
		if(PBA_status == _NOB)
			needGC = 1;
		if(needGC == 1){
			//while(!demand_GC(PBA_status, 'T'))
				//PBA_status++;
		}
		else
			TPA_status = PBA_status * _PPB;
		PBA_status++;
	}
	*t_ppa = TPA_status;
	TPA_status++;
}

