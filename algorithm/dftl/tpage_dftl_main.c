#ifdef UNIT_T
#include "dftl.h"
#include "../../bench/bench.h"

extern algorithm __demand;

extern LINKED_LIST* head; // Head of queue
extern LINKED_LIST* tail; // Tail of queue
extern queue *dftl_q;

extern C_TABLE *CMT; // Cached Mapping Table, Also used as GTD
extern D_TABLE *GTD; // Global Translation Directory, Do not use in tpage_dftl
extern D_OOB *demand_OOB; // Page level OOB
extern D_SRAM *d_sram; // SRAM for contain block data temporarily
extern int tpage_onram_num;

uint32_t demand_get(request *const req){
	request *temp_req;
	if(dftl_q->size != 0){
		temp_req = (request*)q_dequeue(dftl_q);
		switch(temp_req->type){
			case FS_GET_T:
				__demand_get(temp_req);
				break;
			case FS_SET_T:
				__demand_set(temp_req);
				break;
		}
	}
	__demand_get(req);
	return 0;
}

/* demand_get
 * Find page address that req want and load data in a page
 * Search cache to find page address mapping
 * If mapping data is on cache, use that mapping data
 * If mapping data is not on cache, search translation page
 * If translation page found on flash, load whole page to cache
 * Print "Invalid ppa read" when no data written in ppa 
 * Translation page can be loaded even if no data written in ppa
 */ 
uint32_t __demand_get(request *const req){
	bench_algo_start(req); // Algorithm level benchmarking start
	int32_t lpa; // Logical data page address
	int32_t ppa; // Physical data page address
	int32_t t_ppa; // Translation page address
	D_TABLE* p_table; // Contains page table
	value_set *temp_value_set;
	demand_params *params;

	lpa = req->key;
	p_table = CMT_check(lpa, &ppa);

	/* algo_req allocation, initialization */
	params = (demand_params*)malloc(sizeof(demand_params));
	params->type = DATA_R;
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->params = params;
	my_req->end_req = demand_end_req;

	/* Cache hit */
	if(p_table){
		queue_update(CMT[D_IDX].queue_ptr);	// Update CMT queue
		if(ppa != -1){ // Exist mapping in t_page on cache
			bench_algo_end(req); 
			__demand.li->pull_data(ppa, PAGESIZE, req->value, ASYNC, my_req); // Get data in ppa
		}
		else{ // No mapping in t_page on cache
			printf("invalid ppa read\n");
			bench_algo_end(req);
			my_req->end_req(my_req);
		}
	}
	/* Cache miss */
	else{
		t_ppa = CMT[D_IDX].t_ppa; // Get t_ppa
		if(t_ppa != -1){
			if(tpage_onram_num >= MAXTPAGENUM){
				demand_eviction(); // Evict one entry in CMT
			}
			p_table = (D_TABLE*)malloc(PAGESIZE);
			CMT[D_IDX].p_table = p_table;
			CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX)); // Update CMT queue
			CMT[D_IDX].flag = 0; // Set flag in CMT (mapping unchanged)
			/* Load tpage to cache */
			temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(MAPPING_R, temp_value_set, req)); // Get page table
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
	request *temp_req;
	if(dftl_q->size != 0){
		temp_req = (request*)q_dequeue(dftl_q);
		switch(temp_req->type){
			case FS_GET_T:
				__demand_get(temp_req);
				break;
			case FS_SET_T:
				__demand_set(temp_req);
				break;
		}
	}
	__demand_set(req);
	return 0;
}

/* __demand_set
 * Find page address that req want and write data to a page
 * Search cache to find page address mapping
 * If mapping data is on cache, overwrite mapping data in cache
 * If mapping data is not on cache, search translation page
 * If translation page found on flash, load whole page to cache and overwrite mapping data in page
 */ 
uint32_t __demand_set(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	D_TABLE *p_table;
	value_set *temp_value_set;
	demand_params *params;

	/* algo_req allocation, initialization */
	params = (demand_params*)malloc(sizeof(demand_params));
	params->type = DATA_W;
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->params = params;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	p_table = CMT_check(lpa, &ppa);

	/* Cache hit */
	if(p_table){
		demand_OOB[ppa].valid_checker = 0; // Invalidate previous page
		dp_alloc(&ppa); // Allocate data page
		p_table[P_IDX].ppa = ppa; // Page table update
		if(!CMT[D_IDX].flag){ // When flag is set 0
			CMT[D_IDX].flag = 1; // Set flag to 1
			demand_OOB[CMT[D_IDX].t_ppa].valid_checker = 0; // Invalidate tpage in flash
		}
		queue_update(CMT[D_IDX].queue_ptr); // Update CMT queue
		demand_OOB[ppa] = (D_OOB){lpa, 1}; // Update OOB
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, ASYNC, my_req); // Write actual data in ppa
	}
	/* Cache miss */
	else{
		if(tpage_onram_num >= MAXTPAGENUM){
			demand_eviction();
		}
		p_table = (D_TABLE*)malloc(PAGESIZE);
		CMT[D_IDX].p_table = p_table;
		CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX)); // Insert current CMT entry to CMT queue
		CMT[D_IDX].flag = 1; // mapping table changed
		if((t_ppa = CMT[D_IDX].t_ppa) != -1){ // If there is a tpage
			demand_OOB[t_ppa].valid_checker = 0;
			temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(MAPPING_R, temp_value_set, req));
		}
		else{
			for(int i = 0; i < EPP; i++){
				p_table[i].ppa = -1;
			}
			dp_alloc(&ppa);
			p_table[P_IDX].ppa = ppa;
			demand_OOB[ppa] = (D_OOB){lpa, 1};
			bench_algo_end(req);
			__demand.li->push_data(ppa, PAGESIZE, req->value, ASYNC, my_req);
		}
	}
	return 0;
}

/* demand_remove
 * Find page address that req want, erase mapping data, invalidate data page
 * Search cache to find page address mapping
 * If mapping data is on cache, initialize the mapping data
 * If mapping data is not on cache, search translation page
 * If translation page found in flash, load whole page to cache, 
 * whether or not mapping data is on cache search, find translation page and remove mapping data in cache
 * Print Error when there is no such data to erase
 */ 
uint32_t demand_remove(request *const req){
	bench_algo_start(req);
	int32_t lpa;
	int32_t ppa;
	int32_t t_ppa;
	D_TABLE *p_table;
	value_set *temp_value_set;
	request *temp_req = NULL;

	lpa = req->key;
	p_table = CMT_check(lpa, &ppa);
	if(!p_table){
		if(dftl_q->size == 0){
			q_enqueue(req, dftl_q);
			return 0;
		}
		else{
			q_enqueue(req, dftl_q);
			temp_req = (request*)q_dequeue(dftl_q);
			lpa = temp_req->key;
		}
	}
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	if(temp_req){
		my_req->parents = temp_req;
	}
	else{
		my_req->parents = req;
	}
	my_req->end_req = demand_end_req;

	/* Cache hit */
	if(p_table){
		demand_OOB[ppa].valid_checker = 0; // Invalidate data page
		p_table[P_IDX].ppa = -1; // Erase mapping in cache
		CMT[D_IDX].flag = 0;
	}
	/* Cache miss */
	else{
		t_ppa = CMT[D_IDX].t_ppa; // Get t_ppa
		if(t_ppa != -1){
			if(tpage_onram_num >= MAXTPAGENUM)
				demand_eviction();
			/* Load tpage to cache */
			p_table = (D_TABLE*)malloc(PAGESIZE);
			CMT[D_IDX].p_table = p_table;
			temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(MAPPING_R, temp_value_set, req));
			memcpy(p_table, temp_value_set->value, PAGESIZE); // Load page table to CMT
			inf_free_valueset(temp_value_set, FS_MALLOC_R);
			CMT[D_IDX].flag = 0;
			CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX));
			tpage_onram_num++;
			/* Remove mapping data */
			ppa = p_table[P_IDX].ppa;
			if(ppa != -1){
				demand_OOB[ppa].valid_checker = 0; // Invalidate data page
				p_table[P_IDX].ppa = -1; // Remove mapping data
				demand_OOB[t_ppa].valid_checker = 0; // Invalidate tpage on flash
				CMT[D_IDX].flag = 1; // Set CMT flag 1 (mapping changed)
			}
			else
				printf("Error : No such data");
		}
		else
			printf("Error : No such data");
		bench_algo_end(req);
	}
	return 0;
}

/* CMT_check
 * Find mapping data in cache which maps lpa
 * Directly access to mapping data using cache index : O(1)
 * If lpa is found, saves physical page address to ppa, return page table pointer
 * If lpa is not found, return NULL
 */
D_TABLE* CMT_check(int32_t lpa, int32_t *ppa){
	D_TABLE *p_table = CMT[D_IDX].p_table;
	if(p_table)
		*ppa = p_table[P_IDX].ppa;
	return p_table; // return page table pointer
}

/* demand_eviction
 * Evict one translation page on cache
 * Check there is an empty space in cache
 * Find victim cache entry in queue of translation page that loaded on cache
 * If translation page differs from translation page in flash, update translation page in flash
 * If not, just simply erase translation page on cache
 */
uint32_t demand_eviction(){
	int32_t t_ppa;
	C_TABLE *cache_ptr; // Hold pointer that points one cache entry
	D_TABLE *p_table;
	value_set *temp_value_set;

	/* Check capability to load tpage */

	/* Eviction */
	cache_ptr = (C_TABLE*)(tail->DATA); // Save cache_ptr of tail
	p_table = cache_ptr->p_table; // Get page table
	if(cache_ptr->flag != 0){ // When t_page on cache has changed
		if((t_ppa = cache_ptr->t_ppa) != -1) // When it's not a first t_page
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate previous translation page
		/* Write translation page */
		tp_alloc(&t_ppa);
		temp_value_set = inf_get_valueset((PTR)(p_table), FS_MALLOC_W, PAGESIZE);
		__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(MAPPING_W, temp_value_set, NULL));
		demand_OOB[t_ppa] = (D_OOB){(int)(cache_ptr - CMT), 1}; // Update OOB
		cache_ptr->t_ppa = t_ppa; // Update CMT t_ppa
	}
	queue_delete(tail); // Delete CMT entry in queue
	cache_ptr->flag = 0;
	cache_ptr->queue_ptr = NULL;
	free(p_table);
	cache_ptr->p_table = NULL;
	return 0;
}
#endif
