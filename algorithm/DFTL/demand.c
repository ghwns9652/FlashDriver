#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "demand.h"

struct algorithm __demand={
	.create = demand_create,
	.destroy = demand_destroy,
	.get = demand_get,
	.set = demand_set,
	.remove = demand_remove
};

// Please split functions (create, get, set, remove) and (others)

C_TABLE *CMT;
D_TABLE *GTD;
D_OOB *demand_OOB; 
D_SRAM *d_sram;

uint32_t DPA_status = 0;
uint32_t TPA_status = 0;
uint32_t PBA_status = 0;
char needGC = 0;

uint32_t demand_create(lower_info *li, algorithm *algo){
	// Table Alloc 
	GTD = (D_TABLE*)malloc(GTDSIZE);
	CMT = (C_TABLE*)malloc(CMTSIZE);
	demand_OOB = (D_OOB*)malloc(sizeof(D_OOB) * _NOB);
	d_sram = (D_SRAM*)malloc(sizeof(D_SRAM) * _NOB);
	algo->li = li;

	// SRAM Init 
	for(int i = 0; i < GTDENT; i++){
		GTD[i].ppa = -1;
	}
	for(int i = 0; i < CMTENT; i++){
		CMT[i] = (C_TABLE){-1, -1, 0, NULL};
	}
	for(int i = 0; i < _NOB; i++){
		d_sram->lpa_RAM = -1;
		d_sram->PTR_RAM = NULL;
	}
	memset(demand_OOB, 0, sizeof(D_OOB) * _NOB);
}

void demand_destroy(lower_info *li, algorithm *algo){
	free(CMT);
	free(GTD);
	free(demand_OOB);
	free(d_sram);
}

uint32_t demand_get(const request *req){
	uint32_t lpa;
	uint32_t ppa;
	uint32_t t_ppa;
	int CMT_i;
	D_TABLE* p_table;
	demand_params *params = (demand_params*)malloc(sizeof(demand_params));
	params->parents = req;
	params->test = -1;
	
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->end_req = demand_end_req;
	my_req->params = (void*)params;

	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_update(CMT[CMT_i].queue_ptr);
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
	}
	else{
		demand_eviction(&CMT_i);
		t_ppa = GTD[D_IDX].ppa;
		__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, my_req, 0); 
		ppa = p_table[P_IDX].ppa;
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 0, queue_insert((void*)(CMT + CMT_i))};
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
		free(p_table);
	}
}

uint32_t demand_set(const request *req){
	uint32_t lpa; //lpa of data page
	uint32_t ppa; //ppa of data page
	int CMT_i; //index of CMT
	D_TABLE *p_table;
	demand_params *params = (demand_params*)malloc(sizeof(demand_params));
	params->parents = req;
	params->test = -1;

	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->end_req = demand_end_req;
	my_req->params = (void*)params;

	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){ // check CACHE
		// CACHE hit
		demand_OOB[ppa].valid_checker = 0;
		dp_alloc(&ppa);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
		CMT[CMT_i] = (C_TABLE){.ppa = ppa, .flag = 1}; // Is it possible???
		queue_update(CMT[CMT_i].queue_ptr);
	}
	else{
		// CACHE miss
		demand_eviction(&CMT_i);
		dp_alloc(&ppa);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0);
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 1, queue_insert((void*)(CMT + CMT_i))};
		demand_OOB[ppa] = (D_OOB){lpa, 1};
	}
}

bool demand_remove(const request *req){
	uint32_t lpa;
	uint32_t ppa;
	uint32_t t_ppa;
	int CMT_i;
	D_TABLE *p_table;
	
	demand_params *params = (demand_params*)malloc(sizeof(demand_params));
	params->parents = req;
	params->test = -1;

	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->end_req = demand_end_req;
	my_req->params = (void*)params;

	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_delete(CMT[CMT_i].queue_ptr);
		CMT[CMT_i] = (C_TABLE){-1, -1, 0, NULL};
	}
	t_ppa = GTD[D_IDX].ppa;
	__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, my_req, 0);
	ppa = p_table[P_IDX].ppa;
	demand_OOB[ppa].valid_checker = 0;
	p_table[P_IDX].ppa = -1;
	tp_alloc(&t_ppa);
	GTD[D_IDX].ppa = t_ppa;
	__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, my_req, 0);
}

void *demand_end_req(algo_req* input){
	demand_params* params = (demand_params*)input->params;

	request *res = params->parents;
	res->end_req(res);

	free(params);
	free(input);
}

int CMT_check(uint32_t lpa, uint32_t *ppa){
	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].lpa==lpa){
			*ppa = CMT[i].ppa;
			return i; //CMT_i
		}
	}
	return -1; //No such lpa in CMT
}

uint32_t demand_eviction(int *CMT_i){
	uint32_t lpa;
	uint32_t ppa;
	uint32_t t_ppa;
	D_TABLE *p_table;

	/* Check empty entry */
	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].lpa == -1){
			*CMT_i = i;
			return 0;
		}
	}

	/* Eviction */
	*CMT_i = (int)(CMT - (C_TABLE*)(tail->DATA)); //Use tail of queue
	lpa = CMT[*CMT_i].lpa;
	ppa = CMT[*CMT_i].ppa;
	if(CMT[*CMT_i].flag != 0){
		if((t_ppa = GTD[D_IDX].ppa) != -1){	// Check it's first t_page
			__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, NULL, 0);
			p_table[P_IDX].ppa = ppa;
		}
		tp_alloc(&t_ppa);
		__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, NULL, 0);
		demand_OOB[t_ppa] = (D_OOB){D_IDX, 1};
		GTD[D_IDX].ppa = t_ppa;
	}
	queue_delete(tail);
	CMT[*CMT_i] = (C_TABLE){-1, -1, 0, NULL};
	demand_OOB[ppa].valid_checker = 0;
	free(p_table);
}

char btype_check(uint32_t PBA_status){
	for(int i = 0; i < GTDENT; i++){
		if((GTD[i].ppa / _PPB) == PBA_status)
			return 'T';
	}
	return 'D';
}

int lpa_compare(const void *a, const void *b){
	if(((D_SRAM*)a)->lpa_RAM < ((D_SRAM*)b)->lpa_RAM) return -1;
	if(((D_SRAM*)a)->lpa_RAM == ((D_SRAM*)b)->lpa_RAM) return 0;
	if(((D_SRAM*)a)->lpa_RAM > ((D_SRAM*)b)->lpa_RAM) return 1;
}

// PLASE UPDATE (ONLY CMT GC)

// PLEASE CONFIRM GTD MANAGEMENT AND OOB MANAGEMENT
void batch_update(int valid_page_num, uint32_t PBA2PPA){
	uint32_t vba;
	uint32_t t_ppa;
	D_TABLE* p_table;
	uint32_t* temp_lpa_table = (uint32_t*)malloc(sizeof(uint32_t) * _PPB);
	qsort(d_sram, _PPB, sizeof(D_TABLE), lpa_compare);	// Sort d_sram as lpa
	for(int i = 0; i < valid_page_num; i++){	// Make lpa table and SRAM_unload
		temp_lpa_table[i] = d_sram[i].lpa_RAM;
		SRAM_unload(PBA2PPA + i, i);
	}
	
	vba = UINT32_MAX;
	for(int i = 0; i < valid_page_num; i++){
		if(vba == temp_lpa_table[i] / _PPB){
			p_table[temp_lpa_table[i] % _PPB].ppa = PBA2PPA + i;
		}
		else if(vba != UINT32_MAX){
			tp_alloc(&t_ppa);
			__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, NULL, 0);
			demand_OOB[t_ppa] = (D_OOB){vba, 1};
			demand_OOB[GTD[vba].ppa].valid_checker = 0;
			GTD[vba].ppa = t_ppa;
			free(p_table);
			p_table = NULL;
		}
		if(p_table == NULL){
			vba = temp_lpa_table[i] / _PPB;
			__demand.li->pull_data(GTD[vba].ppa, PAGESIZE, (V_PTR)p_table, 0, NULL, 0);
			p_table[temp_lpa_table[i] % _PPB].ppa = PBA2PPA + i;
		}
	}
	free(temp_lpa_table);
}

// Please make NULL ptr to other ptr

void SRAM_load(uint32_t ppa, int idx){
	__demand.li->pull_data(ppa, PAGESIZE, d_sram[idx].PTR_RAM, 0, NULL, 0); // Page load
	d_sram[idx].lpa_RAM = demand_OOB[ppa].reverse_table;	// Page load
	demand_OOB[ppa] = (D_OOB){-1, 0};	// OOB init
}

void SRAM_unload(uint32_t ppa, int idx){
	__demand.li->push_data(ppa, PAGESIZE, d_sram[idx].PTR_RAM, 0, NULL, 0);	// Page unlaod
	demand_OOB[ppa] = (D_OOB){d_sram[idx].lpa_RAM, 1};	// OOB unload
	d_sram[idx] = (D_SRAM){-1, NULL};	// SRAM init
}

// Check the case when no page be GCed.
bool demand_GC(uint32_t victim_PBA, char btype){
	int valid_page_num = 0;	// Valid page num
	uint32_t PBA2PPA = (victim_PBA % _NOB) * _PPB;	// Save PBA to PPA

	/* block type, invalid page check */
	if(btype_check(PBA2PPA) != btype){
		return false;
	}
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
		if(demand_OOB[i].valid_checker != 1){
			return false;
		}
	}

	/* SRAM load */
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){	// Load valid pages to SRAM
		if(demand_OOB[i].valid_checker == 1){
			SRAM_load(i, valid_page_num);
			valid_page_num++;
		}
	}

	/* Block erase */
	__demand.li->trim_block(PBA2PPA, false);

	/* SRAM unlaod */
	if(btype == 'T'){	// Block type check
		for(int j = 0; j < valid_page_num; j++){
			GTD[(d_sram[j].lpa_RAM / _PPB)].ppa = PBA2PPA + j;	// GTD update
			SRAM_unload(PBA2PPA + j, j);	// SRAM unload
		}
		TPA_status = PBA2PPA + valid_page_num;	// TPA_status update
	}
	else if(btype == 'D'){
		batch_update(valid_page_num, PBA2PPA);	// batch_update + t_page update
		DPA_status = PBA2PPA + valid_page_num;	// DPA_status update
	}
	return true;
}

void dp_alloc(uint32_t *ppa){ // Data page allocation
	if(DPA_status % _PPB == 0){
		if(PBA_status == _NOB) 
			needGC = 1;
		if(needGC == 1){
			while(!demand_GC(PBA_status, 'D'))	// Find GC-able d_block and GC
				PBA_status++;
		}
		else
			DPA_status = PBA_status * _PPB;
		PBA_status++;
	}
	*ppa = DPA_status;
	DPA_status++;
}

void tp_alloc(uint32_t *t_ppa){ // Translation page allocation
	if(TPA_status % _PPB == 0){
		if(PBA_status == _NOB) 
			needGC = 1;
		if(needGC == 1){
			while(!demand_GC(PBA_status, 'T'))	// Find GC-able t_block and GC
				PBA_status++;
		}
		else
			TPA_status = PBA_status * _PPB;
		PBA_status++;
	}
	*t_ppa = TPA_status;
	TPA_status++;
}
