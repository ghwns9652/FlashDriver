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
	for(int i = 0; i < GTDENT; i++)
		GTD[i].ppa = -1;
	for(int i = 0; i < CMTENT; i++)
		CMT[i] = (C_TABLE){-1, -1, 0, NULL};
	for(int i = 0; i < _PPB; i++){
		d_sram[i].PTR_RAM = NULL;
		d_sram[i].OOB_RAM = (D_OOB){-1, 0, 0};
	}
	for(int i = 0; i < _NOP; i++)
		demand_OOB[i] = (D_OOB){-1, 0, 0};
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
	while(head)
		queue_delete(head);
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
		if(t_ppa != -1){
			p_table = (D_TABLE*)malloc(PAGESIZE); // p_table allocaiton
			__demand.li->pull_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Get page table
		}
		else{ // lseek error avoid
			printf("Invalid ppa read\n");
			bench_algo_end(req);
			my_req->end_req(my_req);
		}
		ppa = p_table[P_IDX].ppa; // Find ppa
		if(ppa != -1){
			CMT[CMT_i] = (C_TABLE){lpa, ppa, 0, queue_insert((void*)(CMT + CMT_i))}; // CMT update
			free(p_table);
			demand_OOB[ppa].cache_bit = 1;
			bench_algo_end(req); // Algorithm level benchmarking end
			__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Get actual data
		}
		else{ // lseek error avoid
			printf("invalid ppa read\n");
			free(p_table);
			bench_algo_end(req);
			my_req->end_req(my_req);
		}
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
		demand_OOB[ppa] = (D_OOB){lpa, 1, 1}; // Update OOB
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Set actual data
	}

	/* Cache miss */
	else{
		demand_eviction(&CMT_i); // Evict one entry in CMT
		dp_alloc(&ppa);
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 1, queue_insert((void*)(CMT + CMT_i))}; // CMT update
		demand_OOB[ppa] = (D_OOB){lpa, 1, 1};
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
		else{ // p_table initialization
			for(int i = 0; i < EPP; i++)
				p_table[i].ppa = -1;
		}
		if(p_table[P_IDX].ppa != -1)
			demand_OOB[p_table[P_IDX].ppa].valid_checker = 0;
		p_table[P_IDX].ppa = ppa; // Update page table
		tp_alloc(&t_ppa); // Translation page allocation
		__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Set translation page
		demand_OOB[t_ppa] = (D_OOB){D_IDX, 1, 0}; // Update OOB
		GTD[D_IDX].ppa = t_ppa; // Update GTD
		free(p_table);
	}
	demand_OOB[ppa].cache_bit = 0; // Mark data page as t_page mapping
	queue_delete(tail); // Delete queue
	CMT[*CMT_i] = (C_TABLE){-1, -1, 0, NULL}; // Initialize CMT
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
	uint32_t num1 = (uint32_t)(((D_SRAM*)a)->OOB_RAM.reverse_table);
	uint32_t num2 = (uint32_t)(((D_SRAM*)b)->OOB_RAM.reverse_table);
	if(num1 < num2) return -1;
	if(num1 == num2) return 0;
	if(num1 > num2) return 1;
}

void dpage_GC(){
	int32_t lpa;
	int32_t vba;
	int32_t t_ppa;
	int32_t pseudo_ppa;
	int32_t PBA2PPA;
	int valid_page_num;
	int CMT_i;
	D_TABLE* p_table;

	/* Load valid pages to SRAM */
	PBA2PPA = (PBA_status % _NOB) * _PPB; // PPA calculatioin
	valid_page_num = 0;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
		if(demand_OOB[i].valid_checker){ // Load valid pages
			SRAM_load(i, valid_page_num);
			valid_page_num++;
		}
	}
	/* Trim data block */
	__demand.li->trim_block(PBA2PPA, false);

	/* Sort pages in SRAM */
	qsort(d_sram, _PPB, sizeof(D_SRAM), lpa_compare);	// Sort d_sram as lpa
	
	/* Mapping data manage */
	for(int i = 0; i <= valid_page_num; i++){
		lpa = d_sram[i].OOB_RAM.reverse_table;
		/* Cache update */
		if(d_sram[i].OOB_RAM.cache_bit == 1){ // Check cache bit
			CMT_i = CMT_check(lpa, &pseudo_ppa); // Search cache
			CMT[CMT_i].ppa = PBA2PPA + i; // Cache ppa, flag update
			CMT[CMT_i].flag = 1;
		}
		/* Batch update */
		else{
			vba = INT32_MAX; // Initial state
			p_table = NULL;
			if(vba == D_IDX) // Share same vba with previous lpa
				p_table[P_IDX].ppa = PBA2PPA + i;
			else if(vba != INT32_MAX){ // Push t_ppa
				tp_alloc(&t_ppa);
				__demand.li->push_data(t_ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0);
				demand_OOB[t_ppa] = (D_OOB){vba, 1}; // OOB management
				demand_OOB[GTD[vba].ppa].valid_checker = 0; // Invalidate previous tpage
				GTD[vba].ppa = t_ppa; // GTD update
				free(p_table);
				p_table = NULL;
			}
			if(p_table == NULL && i != valid_page_num){ // Start to make new tpage
				vba = D_IDX; // vba calculation
				p_table = (D_TABLE*)malloc(PAGESIZE);
				__demand.li->pull_data(GTD[vba].ppa, PAGESIZE, (V_PTR)p_table, 0, assign_pseudo_req(), 0); // Load tpage
				p_table[P_IDX].ppa = PBA2PPA + i; // Update tpage
			}
		}
	}
	for(int i = 0; i < valid_page_num; i++){
		SRAM_unload(PBA2PPA + i, i); // Unload dpages
	}
	DPA_status = PBA2PPA + valid_page_num;	// DPA_status update
}

int ppa_compare(const void *a, const void *b){
	if(((D_TABLE*)a)->ppa < ((D_TABLE*)b)->ppa) return -1;
	if(((D_TABLE*)a)->ppa == ((D_TABLE*)b)->ppa) return 0;
	if(((D_TABLE*)a)->ppa > ((D_TABLE*)b)->ppa) return 1;
}

/* Please enhance the full merge algorithm !!! */
void tpage_GC(){
	int n = 0;
	int32_t head_ppa;
	int32_t temp_ppa;
	int32_t last_block_idx;
	D_TABLE* GTDcpy = (D_TABLE*)malloc(GTDSIZE);
	memcpy(GTDcpy, GTD, GTDSIZE);
	qsort(GTDcpy, GTDENT, sizeof(D_TABLE), ppa_compare);

	for(int i = 0; i < GTDENT; i++){
		if(GTDcpy[i].ppa == -1)
			continue;
		else{ //
			SRAM_load(GTDcpy[i].ppa, n);
			if(n == _PPB){
				head_ppa = GTD[d_sram[0].OOB_RAM.reverse_table].ppa;
				head_ppa = head_ppa - head_ppa % _PPB;
				last_block_idx = i;
				__demand.li->trim_block(head_ppa, false);
				for(int j = 0; j < _PPB; j++){
					GTD[d_sram[j].OOB_RAM.reverse_table].ppa = head_ppa + j;
					SRAM_unload(head_ppa + j, j);
				}
				n = -1;
			}
			n++;
		}
	}
	if(n > 0){
		head_ppa = GTD[d_sram[0].OOB_RAM.reverse_table].ppa;
		head_ppa = head_ppa - head_ppa % _PPB;
		__demand.li->trim_block(head_ppa, false);
		for(int j = 0; j < n; j++){
			GTD[d_sram[j].OOB_RAM.reverse_table].ppa = head_ppa + j;
			SRAM_unload(head_ppa + j, j);
		}
		TPA_status = head_ppa + n;
		n = 0;
	}
	else{
		for(int i = last_block_idx + 1; i < GTDENT; i++){
			temp_ppa = GTDcpy[i].ppa - (GTDcpy[i].ppa % _PPB);
			if(head_ppa != temp_ppa){
				__demand.li->trim_block(temp_ppa, false);
				TPA_status = temp_ppa;
				break;
			}
		}
	}
	free(GTDcpy);
}

void SRAM_load(int32_t ppa, int idx){
	d_sram[idx].PTR_RAM = (PTR)malloc(PAGESIZE);
	__demand.li->pull_data(ppa, PAGESIZE, d_sram[idx].PTR_RAM, 0, assign_pseudo_req(), 0); // Page load
	d_sram[idx].OOB_RAM = demand_OOB[ppa];	// Page load
	demand_OOB[ppa] = (D_OOB){-1, 0, 0};	// OOB init
}

void SRAM_unload(int32_t ppa, int idx){
	__demand.li->push_data(ppa, PAGESIZE, d_sram[idx].PTR_RAM, 0, assign_pseudo_req(), 0);	// Page unlaod
	free(d_sram[idx].PTR_RAM);
	demand_OOB[ppa] = d_sram[idx].OOB_RAM;	// OOB unload
	d_sram[idx].PTR_RAM = NULL;	// SRAM init
	d_sram[idx].OOB_RAM = (D_OOB){-1, 0, 0};
}

// Check the case when no page be GCed.
bool demand_GC(char btype){
	int32_t PBA2PPA = (PBA_status % _NOB) * _PPB;	// Change PBA to PPA
	char victim_btype;
	victim_btype = btype_check(PBA_status % _NOB);
	/* Case 0 : victim block type 'N' */
	if(victim_btype == 'N'){
		__demand.li->trim_block(PBA2PPA, false);
		if(btype == 'T')
			TPA_status = PBA2PPA;
		else
			DPA_status = PBA2PPA;
		return true;
	}
	else{
		/* Case 1 : victim block type 'T' */
		if(btype == 'T'){
			tpage_GC();
			return true;
		}
		/* Case 2 : victim block type 'D' */
		else{
			/* Check GCability */
			if(victim_btype != 'D')
				return false;
			for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
				if(!demand_OOB[i].valid_checker)
					break;
				else if(i == PBA2PPA + _PPB - 1)
					return false;
			}
			/* Data block GC */
			dpage_GC();	// batch_update + t_page update
			return true;
		}
	}
	return false;
}

void dp_alloc(int32_t *ppa){ // Data page allocation
	if(DPA_status % _PPB == 0){
		if(PBA_status == _NOB) 
			needGC = 1;
		if(needGC == 1){
			while(!demand_GC('D'))	// Find GC-able d_block and GC
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
			while(!demand_GC('T'))	// Find GC-able t_block and GC
				PBA_status++;
		}
		else
			TPA_status = PBA_status * _PPB;
		PBA_status++;
	}
	*t_ppa = TPA_status;
	TPA_status++;
}
