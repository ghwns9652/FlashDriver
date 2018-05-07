#ifdef UNIT_T
#include "dftl.h"
#include "../../bench/bench.h"

extern algorithm __demand;

extern LINKED_LIST* head; // Head of queue
extern LINKED_LIST* tail; // Tail of queue

extern C_TABLE *CMT; // Cached Mapping Table, Also used as GTD
extern D_TABLE *GTD; // Global Translation Directory, Do not use in tpage_dftl
extern D_OOB *demand_OOB; // Page level OOB
extern D_SRAM *d_sram; // SRAM for contain block data temporarily
extern int tpage_onram_num;

/* demand_get
 * Find page address that req want and load data in a page
 * Search cache to find page address mapping
 * If mapping data is on cache, use that mapping data
 * If mapping data is not on cache, search translation page
 * If translation page found on flash, load whole page to cache
 * Print "Invalid ppa read" when no data written in ppa 
 * Translation page can be loaded even if no data written in ppa
 */ 
uint32_t demand_get(request *const req){
	bench_algo_start(req); // Algorithm level benchmarking start
	int32_t lpa; // Logical data page address
	int32_t ppa; // Physical data page address
	int32_t t_ppa; // Translation page address
	D_TABLE* p_table; // Contains page table
	value_set *temp_value_set;
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	lpa = req->key;
	/* Cache hit */
	if(CMT_check(lpa, &ppa)){
		queue_update(CMT[D_IDX].queue_ptr);	// Update CMT queue k:move to cache
		if(ppa != -1){ // Exist mapping in t_page on cache
			bench_algo_end(req); 
			__demand.li->pull_data(ppa, PAGESIZE, req->value, 1, my_req); // Get data in ppa
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
			demand_eviction(); // Evict one entry in CMT
			CMT[D_IDX].flag = 0; // Set flag in CMT (mapping unchanged)
			CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX)); // Update CMT queue
			/* Load tpage to cache */
			p_table = (D_TABLE*)malloc(PAGESIZE);
			CMT[D_IDX].p_table = p_table;
			temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Get page table
			memcpy(p_table, temp_value_set->value, PAGESIZE); // Load page table to cache
			inf_free_valueset(temp_value_set, DMAREAD);
			tpage_onram_num++; // number of tpage on cache +1
			/* Load data page */
			ppa = p_table[P_IDX].ppa; // Find ppa
			if(ppa != -1){
				bench_algo_end(req); 
				__demand.li->pull_data(ppa, PAGESIZE, req->value, 1, my_req); // Get actual data
			}
			else{ // lseek error avoid
				printf("invalid ppa read\n");
				//k : req->type=FS_NOTFOUND_T로 바꿀것
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

/* demand_set
 * Find page address that req want and write data to a page
 * Search cache to find page address mapping
 * If mapping data is on cache, overwrite mapping data in cache
 * If mapping data is not on cache, search translation page
 * If translation page found on flash, load whole page to cache and overwrite mapping data in page
 */ 
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
	if((p_table = CMT_check(lpa, &ppa))){
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
		__demand.li->push_data(ppa, PAGESIZE, req->value, 1, my_req); // Write actual data in ppa
	}
	/* Cache miss */
	else{
		demand_eviction();
		CMT[D_IDX].flag = 1; // mapping table changed
		CMT[D_IDX].queue_ptr = queue_insert((void*)(CMT + D_IDX)); // Insert current CMT entry to CMT queue
		t_ppa = CMT[D_IDX].t_ppa;
		p_table = (D_TABLE*)malloc(PAGESIZE);
		CMT[D_IDX].p_table = p_table;
		/* Load t_page or make new t_page */
		if(t_ppa != -1){ // Load t_page to cache
			temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req());
			memcpy(p_table, temp_value_set->value, PAGESIZE); // Load page table to cache
			inf_free_valueset(temp_value_set, DMAREAD);
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate tpage on flash
		}
		/* Make new page table and load to CMT */
		else{ 
			for(int i = 0; i < EPP; i++)
				p_table[i].ppa = -1;
		}
		tpage_onram_num++; // Number of tpage on cache +1
		ppa = p_table[P_IDX].ppa; // Get ppa in page table
		if(ppa != -1)
			demand_OOB[ppa].valid_checker = 0; // Invalidate previous ppa
		/* Write data page */
		dp_alloc(&ppa); // Data page allocation
		p_table[P_IDX].ppa = ppa; // Update page mapping table in CMT
		demand_OOB[ppa] = (D_OOB){lpa, 1}; // Update OOB
		bench_algo_end(req);
		__demand.li->push_data(ppa, PAGESIZE, req->value, 1, my_req); // Write actual data in ppa
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
	
	/* algo_req allocation, initialization */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = demand_end_req;

	/* Cache hit */
	lpa = req->key;
	if((p_table = CMT_check(lpa, &ppa))){
		demand_OOB[ppa].valid_checker = 0; // Invalidate data page
		p_table[P_IDX].ppa = -1; // Erase mapping in cache
		//k:change flag data
	}
	/* Cache miss */
	else{
		t_ppa = CMT[D_IDX].t_ppa; // Get t_ppa
		if(t_ppa != -1){
			demand_eviction();
			/* Load tpage to cache */
			p_table = (D_TABLE*)malloc(PAGESIZE);
			CMT[D_IDX].p_table = p_table;
			temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
			__demand.li->pull_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req());
			memcpy(p_table, temp_value_set->value, PAGESIZE); // Load page table to CMT
			inf_free_valueset(temp_value_set, DMAREAD);
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
	if(tpage_onram_num < MAXTPAGENUM)
		return 0;

	/* Eviction */
	cache_ptr = (C_TABLE*)(tail->DATA); // Save cache_ptr of tail
	p_table = cache_ptr->p_table; // Get page table
	if(cache_ptr->flag != 0){ // When t_page on cache has changed
		if((t_ppa = cache_ptr->t_ppa) != -1) // When it's not a first t_page
			demand_OOB[t_ppa].valid_checker = 0; // Invalidate previous translation page
		/* Write translation page */
		temp_value_set = inf_get_valueset((PTR)(p_table), DMAWRITE, PAGESIZE);
		tp_alloc(&t_ppa);
		__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Get page table
		inf_free_valueset(temp_value_set, DMAWRITE);
		demand_OOB[t_ppa] = (D_OOB){(int)(cache_ptr - CMT), 1}; // Update OOB
		cache_ptr->t_ppa = t_ppa; // Update CMT t_ppa
	}
	queue_delete(tail); // Delete CMT entry in queue
	cache_ptr->flag = 0;
	cache_ptr->queue_ptr = NULL;
	free(p_table);
	cache_ptr->p_table = NULL;
	tpage_onram_num--; // Number of tpage on cache -1
	return 0;
}
#endif
