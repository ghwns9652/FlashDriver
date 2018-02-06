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

// Please split functions (create, get, set, remove) and (others)

extern LINKED_LIST* head; // Head of queue
extern LINKED_LIST* tail; // Tail of queue

C_TABLE *CMT; // Cached Mapping Table
D_TABLE *GTD; // Global Translation Directory
D_OOB *demand_OOB; // Page level OOB
D_SRAM *d_sram; // SRAM for contain block data temporarily

int32_t DPA_status; // Data page allocation
int32_t TPA_status; // Translation page allocation
int32_t PBA_status; // Block allocation
int8_t needGC; // Indicates need of GC

uint32_t demand_create(lower_info *li, algorithm *algo){
	// Table Allocation
	GTD = (D_TABLE*)malloc(GTDSIZE);
	CMT = (C_TABLE*)malloc(CMTSIZE);
	demand_OOB = (D_OOB*)malloc(sizeof(D_OOB) * _NOP);
	d_sram = (D_SRAM*)malloc(sizeof(D_SRAM) * _PPB);
	algo->li = li;

	// SRAM, OOB initialization
	for(int i = 0; i < GTDENT; i++){
		GTD[i].ppa = -1;
	}
	for(int i = 0; i < CMTENT; i++){
		CMT[i] = (C_TABLE){-1, -1, 0, NULL};
	}
	for(int i = 0; i < _PPB; i++){
		d_sram = (D_SRAM){-1, NULL};
	}
	for(int i = 0; i < _NOP; i++){
		demand_OOB[i] = (D_OOB){-1, 0};
	}
	DPA_status = 0;
	TPA_status = 0;
	PBA_status = 0;
	needGC = 0;
}

void demand_destroy(lower_info *li, algorithm *algo){
	free(CMT);
	free(GTD);
	free(demand_OOB);
	free(d_sram);
}

uint32_t demand_get(request *const req){
	bench_algo_start(req); // Algorithm level benchmarking start
	int32_t lpa; // Logical data page address
	int32_t ppa; // Physical data page address
	int32_t t_ppa; // Translation page address
	int CMT_i; // Cache mapping table index
	D_TABLE* p_table; // Contains page table
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	/* Cache hit */
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_update(CMT[CMT_i].queue_ptr);	// Update queue
		bench_algo_end(req); // Algorithm level benchmarking end
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Get actual data
	}
	/* Cache miss */
	else{
		demand_eviction(&CMT_i); // Evict one entry in CMT
		t_ppa = GTD[D_IDX].ppa; // Get t_ppa
		p_table = (D_TABLE*)malloc(PAGESIZE); // p_table allocaiton
		__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Get page table
		ppa = p_table[P_IDX].ppa; // Find ppa
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 0, queue_insert((void*)(CMT + CMT_i))}; // CMT update
		free(p_table);
		bench_algo_end(req); // Algorithm level benchmarking end
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Get actual data
	}
}

uint32_t demand_set(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int CMT_i;

	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	/* Cache hit */
	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		demand_OOB[ppa].valid_checker = 0; // Invalidate previous page
		dp_alloc(&ppa); // Allocate data page
		CMT[CMT_i].ppa = ppa; // Update ppa
		CMT[CMT_i].flag = 1; // Mapping table changed
		queue_update(CMT[CMT_i].queue_ptr); // Update queue
		demand_OOB[ppa] = (D_OOB){lpa, 1}; // Update OOB
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Set actual data
	}
	/* Cache miss */
	else{
		demand_eviction(&CMT_i); // Evict one entry in CMT
		dp_alloc(&ppa);
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 1, queue_insert((void*)(CMT + CMT_i))}; // CMT update
		demand_OOB[ppa] = (D_OOB){lpa, 1};
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Set actual data
	}
}

uint32_t demand_remove(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	int CMT_i;
	D_TABLE *p_table;
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	/* Cache hit */
	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_delete(CMT[CMT_i].queue_ptr); // Delete queue
		CMT[CMT_i] = (C_TABLE){-1, -1, 0, NULL}; // Initailize CMT
	}

	/* Both cache hit or miss */
	t_ppa = GTD[D_IDX].ppa; // Find t_ppa
	p_table = (D_TABLE*)malloc(PAGESIZE);
	__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Get page table
	ppa = p_table[P_IDX].ppa; // Get ppa
	demand_OOB[ppa].valid_checker = 0; // Invalidate ppa
	p_table[P_IDX].ppa = -1; // Invalidte mapping data
	tp_alloc(&t_ppa); // Translation page allocation
	GTD[D_IDX].ppa = t_ppa; // GTD update
	bench_algo_end(req);
	__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, my_req, 0); // Update page table
	free(p_table);
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
		if(CMT[i].lpa == lpa){ // Find lpa in CMT
			*ppa = CMT[i].ppa; // Return ppa in CMT
			return i; //CMT_i
		}
	}
	return -1; //No lpa in CMT
}

uint32_t demand_eviction(int *CMT_i){
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	D_TABLE *p_table;

	/* Check empty entry */
	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].lpa == -1){
			*CMT_i = i; // Empty entry
			return 0; 
		}
	}

	/* Eviction */
	*CMT_i = (int)((C_TABLE*)(tail->DATA) - CMT); // Save CMT_i of tail
	lpa = CMT[*CMT_i].lpa; // Get lpa of CMT_i
	ppa = CMT[*CMT_i].ppa; // Get ppa of CMT_i
	if(CMT[*CMT_i].flag != 0){ // When CMT_i has changed
		p_table = (D_TABLE*)malloc(PAGESIZE);
		if((t_ppa = GTD[D_IDX].ppa) != -1){	// When it's not a first t_page
			__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Get page table
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate translation page
		}
		p_table[P_IDX].ppa = ppa; // Update page table
		tp_alloc(&t_ppa); // Translation page allocation
		__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Set translation page
		demand_OOB[t_ppa] = (D_OOB){D_IDX, 1}; // Update OOB
		GTD[D_IDX].ppa = t_ppa; // Update GTD
		free(p_table);
	}
	queue_delete(tail); // Delete queue
	CMT[*CMT_i] = (C_TABLE){-1, -1, 0, NULL}; // Initializae CMT
	//demand_OOB[ppa].valid_checker = 0;
}

char btype_check(int32_t PBA_status){
	int32_t PBA2PPA = PBA_status * _PPB;
	int invalid_page_num = 0;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
		if(!demand_OOB[i].valid_checker)
			invalid_page_num++;
	}
	if(invalid_page_num == _PPB)
		return 'N';

	for(int i = 0; i < GTDENT; i++){
		if(GTD[i].ppa != -1 && (GTD[i].ppa / _PPB) == PBA_status)
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
void batch_update(int valid_page_num, int32_t PBA2PPA){
	int32_t vba;
	int32_t t_ppa;
	D_TABLE* p_table = (D_TABLE*)malloc(PAGESIZE);
	int32_t* temp_lpa_table = (int32_t*)malloc(sizeof(int32_t) * _PPB);
	qsort(d_sram, _PPB, sizeof(D_TABLE), lpa_compare);	// Sort d_sram as lpa
	for(int i = 0; i < valid_page_num; i++){	// Make lpa table and SRAM_unload
		temp_lpa_table[i] = d_sram[i].lpa_RAM;
		SRAM_unload(PBA2PPA + i, i);
	}
	
	vba = INT32_MAX;
	for(int i = 0; i < valid_page_num; i++){
		if(vba == temp_lpa_table[i] / EPP){
			p_table[temp_lpa_table[i] % EPP].ppa = PBA2PPA + i;
		}
		else if(vba != INT32_MAX){
			tp_alloc(&t_ppa);
			__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0);
			demand_OOB[t_ppa] = (D_OOB){vba, 1};
			demand_OOB[GTD[vba].ppa].valid_checker = 0;
			GTD[vba].ppa = t_ppa;
			free(p_table);
			p_table = NULL;
		}
		if(p_table == NULL){
			vba = temp_lpa_table[i] / EPP;
			__demand.li->pull_data(GTD[vba].ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0);
			p_table[temp_lpa_table[i] % EPP].ppa = PBA2PPA + i;
		}
	}
	free(temp_lpa_table);
}

// Please make NULL ptr to other ptr

void SRAM_load(int32_t ppa, int idx){
	d_sram[idx].PTR_RAM = (PTR)malloc(PAGESIZE);
	__demand.li->pull_data(ppa, PAGESIZE, d_sram[idx].PTR_RAM, 0, assign_pseudo_req(), 0); // Page load
	d_sram[idx].lpa_RAM = demand_OOB[ppa].reverse_table;	// Page load
	demand_OOB[ppa] = (D_OOB){-1, 0};	// OOB init
}

void SRAM_unload(int32_t ppa, int idx){
	__demand.li->push_data(ppa, PAGESIZE, d_sram[idx].PTR_RAM, 0, assign_pseudo_req(), 0);	// Page unlaod
	free(d_sram[idx].PTR_RAM);
	demand_OOB[ppa] = (D_OOB){d_sram[idx].lpa_RAM, 1};	// OOB unload
	d_sram[idx] = (D_SRAM){-1, NULL};	// SRAM init
}

// Check the case when no page be GCed.
bool demand_GC(int32_t victim_PBA, char btype){
	int valid_page_num = 0;	// Valid page num
	int32_t PBA2PPA = (victim_PBA % _NOB) * _PPB;	// Save PBA to PPA
	char victim_btype = btype_check(victim_PBA % _NOB);
	/* block type, invalid page check */
	if(victim_btype != 'N' && victim_btype != btype)
		return false;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
		if(!demand_OOB[i].valid_checker)
			break;
		else if(i == PBA2PPA + _PPB - 1)
			return false;
	}

	/* SRAM load */
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){	// Load valid pages to SRAM
		if(demand_OOB[i].valid_checker){
			SRAM_load(i, valid_page_num);
			valid_page_num++;
		}
	}

	/* Block erase */
	__demand.li->trim_block(PBA2PPA, false);

	/* SRAM unlaod */
	if(btype == 'T'){	// Block type check
		for(int j = 0; j < valid_page_num; j++){
			GTD[(d_sram[j].lpa_RAM)].ppa = PBA2PPA + j;	// GTD update
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

void dp_alloc(int32_t *ppa){ // Data page allocation
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

void tp_alloc(int32_t *t_ppa){ // Translation page allocation
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
