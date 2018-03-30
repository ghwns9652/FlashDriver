#ifdef UNIT_T
#include "dftl.h"
#include "../../bench/bench.h"

extern algorithm __demand;

extern LINKED_LIST* head; // Head of queue
extern LINKED_LIST* tail; // Tail of queue

extern C_TABLE *CMT; // Cached Mapping Table
extern D_TABLE *GTD; // Global Translation Directory
extern D_OOB *demand_OOB; // Page level OOB
extern D_SRAM *d_sram; // SRAM for contain block data temporarily

// lseek error can makes t_page to be uploaded on cache
uint32_t demand_get(request *const req){
	bench_algo_start(req); // Algorithm level benchmarking start
	int32_t lpa; // Logical data page address
	int32_t ppa; // Physical data page address
	int32_t t_ppa; // Translation page address
	int CMT_i; // Cache mapping table index
	D_TABLE* p_table; // Contains page table
	value_set *temp_value_set;
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	/* Cache hit */
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_update(CMT[CMT_i].queue_ptr);	// Update queue
		if(ppa != -1){
			bench_algo_end(req); // Algorithm level benchmarking end
			__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Get actual data
		}
		else{
			printf("invalid ppa read\n");
			bench_algo_end(req);
			my_req->end_req(my_req);
		}
	}
	/* Cache miss */
	else{
		t_ppa = GTD[D_IDX].ppa; // Get t_ppa
		if(t_ppa != -1){
			demand_eviction(&CMT_i); // Evict one entry in CMT
			CMT[CMT_i].GTD_idx = D_IDX;
			CMT[CMT_i].flag = 0;
			CMT[CMT_i].queue_ptr = queue_insert((void*)(CMT + CMT_i));
			p_table = CMT[CMT_i].p_table;
			temp_value_set = inf_get_valueset(NULL, DMAREAD);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0); // Get page table
			memcpy(p_table, temp_value_set->value, PAGESIZE);
			inf_free_valueset(temp_value_set, DMAREAD);
			ppa = p_table[P_IDX].ppa; // Find ppa
			if(ppa != -1){
				bench_algo_end(req); // Algorithm level benchmarking end
				__demand.li->pull_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Get actual data
			}
			else{ // lseek error avoid
				printf("invalid ppa read\n");
				bench_algo_end(req);
				my_req->end_req(my_req);
			}
		}
		else{ // lseek error avoid
			printf("Invalid ppa read\n");
			bench_algo_end(req);
			my_req->end_req(my_req);
		}
	}
}

uint32_t demand_set(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	int CMT_i;
	D_TABLE *p_table;
	value_set *temp_value_set;

	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	/* Cache hit */
	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		demand_OOB[ppa].valid_checker = 0; // Invalidate previous page
		dp_alloc(&ppa); // Allocate data page
		CMT[CMT_i].p_table[P_IDX].ppa = ppa; // Update ppa
		if(!CMT[CMT_i].flag){
			CMT[CMT_i].flag = 1;
			demand_OOB[GTD[D_IDX].ppa].valid_checker = 0;
		}
		queue_update(CMT[CMT_i].queue_ptr); // Update queue
		demand_OOB[ppa] = (D_OOB){lpa, 1}; // Update OOB
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Set actual data
	}
	/* Cache miss */
	else{
		demand_eviction(&CMT_i);
		CMT[CMT_i].GTD_idx = D_IDX;
		CMT[CMT_i].flag = 1;
		CMT[CMT_i].queue_ptr = queue_insert((void*)(CMT + CMT_i));
		t_ppa = GTD[D_IDX].ppa;
		p_table = CMT[CMT_i].p_table;
		// Load t_page or make new t_page
		if(t_ppa != -1){ // Load t_page to cache
			temp_value_set = inf_get_valueset(NULL, DMAREAD);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0);
			memcpy(p_table, temp_value_set->value, PAGESIZE);
			inf_free_valueset(temp_value_set, DMAREAD);
			demand_OOB[t_ppa].valid_checker = 0;
		}
		else{ // Make new t_page
			for(int i = 0; i < EPP; i++)
				p_table[i].ppa = -1;
		}
		ppa = p_table[P_IDX].ppa;
		if(ppa != -1)
			demand_OOB[ppa].valid_checker = 0; // Invalidate previous ppa
		// Update cache mapping table
		dp_alloc(&ppa); // Data page allocation
		p_table[P_IDX].ppa = ppa;
		demand_OOB[ppa] = (D_OOB){lpa, 1}; // Update OOB
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
	value_set *temp_value_set;
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	/* Cache hit */
	lpa = req->key;
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		demand_OOB[ppa].valid_checker = 0;
		CMT[CMT_i].p_table[P_IDX].ppa = -1;
	}
	/* Cache miss */
	else{
		t_ppa = GTD[D_IDX].ppa;
		if(t_ppa != -1){
			demand_eviction(&CMT_i);
			p_table = CMT[CMT_i].p_table;
			CMT[CMT_i].GTD_idx = D_IDX;
			temp_value_set = inf_get_valueset(NULL, DMAREAD);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0);
			memcpy(p_table, temp_value_set->value, PAGESIZE);
			inf_free_valueset(temp_value_set, DMAREAD);
			CMT[CMT_i].flag = 0;
			CMT[CMT_i].queue_ptr = queue_insert((void*)(CMT + CMT_i));
			ppa = p_table[P_IDX].ppa;
			if(ppa != -1){
				demand_OOB[ppa].valid_checker = 0;
				p_table[P_IDX].ppa = -1;
				demand_OOB[t_ppa].valid_checker = 0;
				CMT[CMT_i].flag = 1;
			}
			else
				printf("Error : No such data");
		}
		else
			printf("Error : No such data");
		bench_algo_end(req);
	}
}

int CMT_check(int32_t lpa, int32_t *ppa){
	int GTD_idx = D_IDX;
	int CMT_idx = GTD_idx % CMTENT;
	for(int i = 0; i < CMTENT; i++){
		if(CMT_idx == CMTENT)
			CMT_idx = 0;
		if(CMT[CMT_idx].GTD_idx == GTD_idx){ // Find lpa in CMT
			*ppa = CMT[CMT_idx].p_table[P_IDX].ppa; // Return ppa in CMT
			return CMT_idx; //CMT_i
		}
		CMT_idx++;
	}
	return -1; //No lpa in CMT
}

// Make empty cache entry queue function
uint32_t demand_eviction(int *CMT_i){
	int32_t victim_idx;
	int32_t ppa;
	int32_t t_ppa;
	D_TABLE *p_table;
	value_set *temp_value_set;

	/* Check empty entry */ // HOW TO ENHENCE EMPTY ENTRY CHECK CODE??
	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].GTD_idx == -1){
			*CMT_i = i; // Empty entry
			return 0; 
		}
	}
	/* Eviction */
	*CMT_i = (int)((C_TABLE*)(tail->DATA) - CMT); // Save CMT_i of tail
	victim_idx = CMT[*CMT_i].GTD_idx; // Get lpa of CMT_i
	if(CMT[*CMT_i].flag != 0){ // When CMT_i has changed
		if((t_ppa = GTD[victim_idx].ppa) != -1)	// When it's not a first t_page
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate translation page
		temp_value_set = inf_get_valueset((PTR)CMT[*CMT_i].p_table, DMAWRITE);
		tp_alloc(&t_ppa);
		__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0); // Get page table
		inf_free_valueset(temp_value_set, DMAWRITE);
		demand_OOB[t_ppa] = (D_OOB){victim_idx, 1}; // Update OOB
		GTD[victim_idx].ppa = t_ppa; // Update GTD
	}
	queue_delete(tail); // Delete queue
	CMT[*CMT_i].GTD_idx = -1;
	CMT[*CMT_i].flag = 0;
	CMT[*CMT_i].queue_ptr = NULL;
}
#endif
