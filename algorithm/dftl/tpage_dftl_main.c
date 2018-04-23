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
extern int tpage_onram_num;

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
	if(CMT_check(lpa, &ppa)){
		queue_update(CMT[D_IDX].queue_ptr);	// Update queue
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
		t_ppa = CMT[D_IDX].t_ppa; // Get t_ppa
		if(t_ppa != -1){
			demand_eviction(); // Evict one entry in CMT
			CMT[D_IDX].flag = 0;
			CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX));
			p_table = (D_TABLE*)malloc(PAGESIZE);
			CMT[D_IDX].p_table = p_table;
			temp_value_set = inf_get_valueset(NULL, DMAREAD);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0); // Get page table
			memcpy(p_table, temp_value_set->value, PAGESIZE);
			inf_free_valueset(temp_value_set, DMAREAD);
			tpage_onram_num++;
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
	D_TABLE *p_table;
	value_set *temp_value_set;

	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	/* Cache hit */
	if(p_table = CMT_check(lpa, &ppa)){
		demand_OOB[ppa].valid_checker = 0; // Invalidate previous page
		dp_alloc(&ppa); // Allocate data page
		p_table[P_IDX].ppa = ppa;
		if(!CMT[D_IDX].flag){
			CMT[D_IDX].flag = 1;
			demand_OOB[CMT[D_IDX].t_ppa].valid_checker = 0;
		}
		queue_update(CMT[D_IDX].queue_ptr); // Update queue
		demand_OOB[ppa] = (D_OOB){lpa, 1}; // Update OOB
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 0, my_req, 0); // Set actual data
	}
	/* Cache miss */
	else{
		demand_eviction();
		CMT[D_IDX].flag = 1;
		CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX));
		t_ppa = CMT[D_IDX].t_ppa;
		p_table = (D_TABLE*)malloc(PAGESIZE);
		CMT[D_IDX].p_table = p_table;
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
		tpage_onram_num++;
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
	if(p_table = CMT_check(lpa, &ppa)){
		demand_OOB[ppa].valid_checker = 0;
		p_table[P_IDX].ppa = -1;
	}
	/* Cache miss */
	else{
		t_ppa = CMT[D_IDX].t_ppa;
		if(t_ppa != -1){
			demand_eviction();
			p_table = (D_TABLE*)malloc(PAGESIZE);
			CMT[D_IDX].p_table = p_table;
			temp_value_set = inf_get_valueset(NULL, DMAREAD);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0);
			memcpy(p_table, temp_value_set->value, PAGESIZE);
			inf_free_valueset(temp_value_set, DMAREAD);
			CMT[D_IDX].flag = 0;
			CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX));
			tpage_onram_num++;
			ppa = p_table[P_IDX].ppa;
			if(ppa != -1){
				demand_OOB[ppa].valid_checker = 0;
				p_table[P_IDX].ppa = -1;
				demand_OOB[t_ppa].valid_checker = 0;
				CMT[D_IDX].flag = 1;
			}
			else
				printf("Error : No such data");
		}
		else
			printf("Error : No such data");
		bench_algo_end(req);
	}
}

D_TABLE* CMT_check(int32_t lpa, int32_t *ppa){
	D_TABLE *p_table = CMT[D_IDX].p_table;
	if(p_table)
		*ppa = p_table[P_IDX].ppa;
	return p_table;
}

// Make empty cache entry queue function
uint32_t demand_eviction(){
	int32_t victim_idx;
	int32_t ppa;
	int32_t t_ppa;
	C_TABLE *cache_ptr;
	D_TABLE *p_table;
	value_set *temp_value_set;

	/* Check capability to load tpage */
	if(tpage_onram_num < MAXTPAGENUM)
		return 0;

	/* Eviction */
	cache_ptr = (C_TABLE*)(tail->DATA); // Save cache_ptr of tail
	p_table = cache_ptr->p_table;
	if(cache_ptr->flag != 0){ // When CMT_i has changed
		if((t_ppa = cache_ptr->t_ppa) != -1)	// When it's not a first t_page
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate translation page
		temp_value_set = inf_get_valueset((PTR)(p_table), DMAWRITE);
		tp_alloc(&t_ppa);
		__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set, 0, assign_pseudo_req(), 0); // Get page table
		inf_free_valueset(temp_value_set, DMAWRITE);
		demand_OOB[t_ppa] = (D_OOB){(int)(cache_ptr - CMT), 1}; // Update OOB
		cache_ptr->t_ppa = t_ppa; // Update GTD
	}
	queue_delete(tail); // Delete queue
	cache_ptr->flag = 0;
	cache_ptr->queue_ptr = NULL;
	free(p_table);
	cache_ptr->p_table = NULL;
	tpage_onram_num--;
}
#endif
