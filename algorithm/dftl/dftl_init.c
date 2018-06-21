#include "dftl.h"

struct algorithm __demand={
	.create = demand_create,
	.destroy = demand_destroy,
	.get = demand_get,
	.set = demand_set,
	.remove = demand_remove
};
/*
   data에 관한 write buffer를 생성
   128개의 channel이라서 128개를 한번에 처리가능
   1024개씩 한번에 쓰도록.(dynamic)->변수처리
   ppa는 1씩 증가해서 보내도됨.
 */

extern LINKED_LIST* head;
extern LINKED_LIST* tail;
queue *dftl_q;

C_TABLE *CMT; // Cached Mapping Table
D_TABLE *GTD; // Global Translation Directory
D_OOB *demand_OOB; // Page level OOB
D_SRAM *d_sram; // SRAM for contain block data temporarily

pthread_mutex_t cache_mutex;
//extern pthread_mutex_t test_lock;

/*
k:
   block 영역 분리를 확실히 할것.
   reserved block을 하나 둬서 gc block을 trim 한 다음 두개를 교환하는 식으로
   gc 구현
 */

int32_t DPA_status; // Data page allocation
int32_t TPA_status; // Translation page allocation
int32_t PBA_status; // Block allocation
int8_t needGC; // Indicates need of GC
int32_t tpage_onram_num; // Number of translation page on cache

/* demand_create
 * Initialize data structures that are used in DFTL
 * Initialize global variables
 * Use different code according to DFTL version
 */
uint32_t demand_create(lower_info *li, algorithm *algo){
	// Table Allocation
#ifdef UNIT_D
	GTD = (D_TABLE*)malloc(GTDSIZE);
#endif
	CMT = (C_TABLE*)malloc(CMTSIZE);
	demand_OOB = (D_OOB*)malloc(sizeof(D_OOB) * _NOP);
	d_sram = (D_SRAM*)malloc(sizeof(D_SRAM) * _PPB);
	algo->li = li;

#ifdef UNIT_D
	// SRAM, OOB initialization
	for(int i = 0; i < GTDENT; i++)
		GTD[i].ppa = -1;
	for(int i = 0; i < CMTENT; i++)
		CMT[i] = (C_TABLE){-1, -1, 0, NULL};
	for(int i = 0; i < _PPB; i++){
		d_sram[i].valueset_RAM = NULL;
		d_sram[i].OOB_RAM = (D_OOB){-1, 0};
	}
	for(int i = 0; i < _NOP; i++)
		demand_OOB[i] = (D_OOB){-1, 0};
#endif
// k: else or elif
#ifdef UNIT_T
	// SRAM, OOB initialization
	for(int i = 0; i < CMTENT; i++){
		CMT[i].t_ppa = -1;
		CMT[i].p_table = NULL;
		CMT[i].flag = 0;
		CMT[i].queue_ptr = NULL;
	}
	for(int i = 0; i < _PPB; i++){
	    d_sram[i].valueset_RAM = NULL;
		d_sram[i].OOB_RAM = (D_OOB){-1, 0};
	}
	for(int i = 0; i < _NOP; i++)
		demand_OOB[i] = (D_OOB){-1, 0};
#endif
	DPA_status = 0;
	TPA_status = 0;
	PBA_status = 0;
	needGC = 0;
	tpage_onram_num = 0;

	pthread_mutex_init(&cache_mutex, NULL);
	q_init(&dftl_q, 1024);
	return 0;
}

// When FTL is destroyed, move CMT to SSD
/* demand_destroy
 * Free data structures that are used in DFTL
 */
void demand_destroy(lower_info *li, algorithm *algo){
	while(head){
		free(((C_TABLE*)(head->DATA))->p_table);
		queue_delete(head);
	}
	q_free(dftl_q);
	free(CMT);
	free(GTD);
	free(demand_OOB);
	free(d_sram);
}

/* demand_end_req
 * Free my_req from interface level
 */
/* Does it need to return void pointer??? */
void *demand_end_req(algo_req* input){
	demand_params *params = (demand_params*)input->params;
	value_set *temp_v = params->value;
	request *res = input->parents;
	D_TABLE *dest;
	D_TABLE *src;
	int32_t lpa;
	if(res){
		lpa = res->key;
	}
	switch(params->type){
		case DATA_R:
			if(res)
				res->end_req(res);
			break;
		case DATA_W:
			if(res)
				res->end_req(res);
			break;
		case MAPPING_R:
			if(CMT[D_IDX].t_ppa != -1 && CMT[D_IDX].p_table){
				dest = CMT[D_IDX].p_table;
				src = (D_TABLE*)(temp_v->value);
				printf("src\n");
				cache_show((char*)src);
				memcpy(dest, src, PAGESIZE);
				/*
				for(int i = 0; i < EPP; i++){
					if(dest[i].ppa == -1){
						dest[i].ppa = src[i].ppa;
					}
					else if(src[i].ppa != -1){
						demand_OOB[src[i].ppa].valid_checker = 0;
					}
				}
				*/
				CMT[D_IDX].t_ppa = -1;
			}
#if ASYNC
			if(res && (res->type == FS_GET_T)){ // queueing only get requests
				q_enqueue(res, dftl_q);
			}
#endif
			printf("dest\n");
			cache_show((char*)dest);
			inf_free_valueset(temp_v, FS_MALLOC_R);
			break;
		case MAPPING_W:
			tpage_onram_num--;
		//	cache_show(temp_v->value);
			inf_free_valueset(temp_v, FS_MALLOC_W);
			break;
		case GC_R:
			break;
		case GC_W:
			break;
	}
	free(params);
	free(input);
	//pthread_mutex_unlock(&test_lock);
	return NULL;
}

/* assign_pseudo_req
 * Make pseudo_my_req
 * psudo_my_req is req from algorithm, not from the interface
 */

algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req){
	static int seq=0;
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	pseudo_my_req->parents = req;
	demand_params *params = (demand_params*)malloc(sizeof(demand_params));//allocation;
	params->test=seq++;
	params->type = type;
	params->value = temp_v;
	switch(type){
		case DATA_R:
			break;
		case DATA_W:
			break;
		case MAPPING_R:
			break;
		case MAPPING_W:
			break;
		case GC_W:
			break;
		case GC_R:
			break;
	}
	pseudo_my_req->end_req = demand_end_req;
	pseudo_my_req->params = params;
	return pseudo_my_req;
}

/* dp_alloc
 * Find allocatable page address for data page allocation
 * Guaranteed to search block linearly to find allocatable page address (from 0 to _NOB)
 * Saves allocatable page address to ppa
 */
void dp_alloc(int32_t *ppa){ // Data page allocation
	if(DPA_status % _PPB == 0){
		if(PBA_status == _NOB) 
			needGC = 1;
		if(needGC == 1){
			/*
			 영역을 나누면 linear search 불필요
			 translation block -> using heap
			 data block -> too much to use linear search, using heap
			 */
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

/* tp_alloc
 * Find allocatable page address for translation page allocation
 * Guaranteed to search block linearly to find allocatable page address (from 0 to _NOB)
 * Saves allocatable page address to t_ppa
 */
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
