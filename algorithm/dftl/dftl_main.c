#ifdef UNIT_D
#include "dftl.h"
#include "../../bench/bench.h"

extern algorithm __demand;

extern LINKED_LIST* head; // Head of queue
extern LINKED_LIST* tail; // Tail of queue

extern C_TABLE *CMT; // Cached Mapping Table
extern D_TABLE *GTD; // Global Translation Directory
extern D_OOB *demand_OOB; // Page level OOB
extern D_SRAM *d_sram; // SRAM for contain block data temporarily

int set_count = 0;
int eviction_count = 0;

uint32_t demand_get(request *const req){
	bench_algo_start(req); // Algorithm level benchmarking start
	int32_t lpa; // Logical data page address
	int32_t ppa; // Physical data page address
	int32_t t_ppa; // Translation page address
	int CMT_i; // Cache mapping table index
	D_TABLE* p_table; // Contains page table
	value_set* temp_value_set;
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	/* Cache hit */
	if((CMT_i = CMT_check(lpa, &ppa)) != -1){
		queue_update(CMT[CMT_i].queue_ptr);	// Update queue
		bench_algo_end(req); // Algorithm level benchmarking end
		__demand.li->pull_data(ppa, PAGESIZE, req->value, 1, my_req); // Get actual data
	}
	/* Cache miss */
	else{
		demand_eviction(&CMT_i); // Evict one entry in CMT
		t_ppa = GTD[D_IDX].ppa; // Get t_ppa
		if(t_ppa != -1){
			temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Get page table
			p_table = (D_TABLE*)temp_value_set->value;
			ppa = p_table[P_IDX].ppa; // Find ppa
			if(ppa != -1){
				CMT[CMT_i] = (C_TABLE){lpa, ppa, 0, queue_insert((void*)(CMT + CMT_i))}; // CMT update
				inf_free_valueset(temp_value_set, DMAREAD);
				demand_OOB[ppa].cache_bit = 1;
				bench_algo_end(req); // Algorithm level benchmarking end
				__demand.li->pull_data(ppa, PAGESIZE, req->value, 1, my_req); // Get actual data
			}
			else{ // lseek error avoid
				printf("invalid ppa read\n");
				inf_free_valueset(temp_value_set, DMAREAD);
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
	return 0;
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
		__demand.li->push_data(ppa, PAGESIZE, req->value, 1, my_req); // Set actual data
	}

	/* Cache miss */
	else{
		demand_eviction(&CMT_i); // Evict one entry in CMT
		dp_alloc(&ppa);
		CMT[CMT_i] = (C_TABLE){lpa, ppa, 1, queue_insert((void*)(CMT + CMT_i))}; // CMT update
		demand_OOB[ppa] = (D_OOB){lpa, 1, 1};
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 1, my_req); // Set actual data
	}
	return 0;
}

uint32_t demand_remove(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	int CMT_i;
	D_TABLE *p_table;
	value_set *temp_value_set;
	value_set *temp_value_set2;
	
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
	temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
	__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Get page table
	p_table = (D_TABLE*)temp_value_set->value;
	ppa = p_table[P_IDX].ppa; // Get ppa
	demand_OOB[ppa].valid_checker = 0; // Invalidate ppa
	p_table[P_IDX].ppa = -1; // Invalidte mapping data
	tp_alloc(&t_ppa); // Translation page allocation
	GTD[D_IDX].ppa = t_ppa; // GTD update
	temp_value_set2 = inf_get_valueset(temp_value_set->value, DMAWRITE, PAGESIZE);
	__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set2, 1, my_req); // Update page table
	inf_free_valueset(temp_value_set, DMAREAD);
	inf_free_valueset(temp_value_set2, DMAWRITE);
	bench_algo_end(req);
	return 0;
}

int CMT_check(int32_t lpa, int32_t *ppa){
	int CMT_idx = lpa % CMTENT;
	for(int i = 0; i < CMTENT; i++){
		if(CMT_idx == CMTENT)
			CMT_idx = 0;
		if(CMT[CMT_idx].lpa == lpa){ // Find lpa in CMT
			*ppa = CMT[CMT_idx].ppa; // Return ppa in CMT
			return CMT_idx; //CMT_i
		}
		CMT_idx++;
	}
	return -1; //No lpa in CMT
}

uint32_t demand_eviction(int *CMT_i){
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	D_TABLE *p_table;
	value_set *temp_value_set;
	value_set *temp_value_set2;

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
		if((t_ppa = GTD[D_IDX].ppa) != -1){	// When it's not a first t_page
			temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Get page table
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate translation page
			temp_value_set2 = inf_get_valueset(temp_value_set->value, DMAWRITE, PAGESIZE);
			inf_free_valueset(temp_value_set, DMAREAD);
			p_table = (D_TABLE*)temp_value_set2->value;
		}
		else{ // p_table initialization
			temp_value_set2 = inf_get_valueset(NULL, DMAWRITE, PAGESIZE);
			p_table = (D_TABLE*)temp_value_set2->value;
			for(int i = 0; i < EPP; i++)
				p_table[i].ppa = -1;
		}
		if(p_table[P_IDX].ppa != -1)
			demand_OOB[p_table[P_IDX].ppa].valid_checker = 0;
		p_table[P_IDX].ppa = ppa; // Update page table
		tp_alloc(&t_ppa); // Translation page allocation
		__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set2, 1, assign_pseudo_req()); // Set translation page
		demand_OOB[t_ppa] = (D_OOB){D_IDX, 1, 0}; // Update OOB
		GTD[D_IDX].ppa = t_ppa; // Update GTD
		inf_free_valueset(temp_value_set2, DMAWRITE);
	}
	demand_OOB[ppa].cache_bit = 0; // Mark data page as t_page mapping
	queue_delete(tail); // Delete queue
	CMT[*CMT_i] = (C_TABLE){-1, -1, 0, NULL}; // Initialize CMT
	return 0;
}
#endif
