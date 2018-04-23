#include "dftl.h"

struct algorithm __demand={
	.create = demand_create,
	.destroy = demand_destroy,
	.get = demand_get,
	.set = demand_set,
	.remove = demand_remove
};

extern LINKED_LIST* head;
extern LINKED_LIST* tail;

C_TABLE *CMT; // Cached Mapping Table
D_TABLE *GTD; // Global Translation Directory
D_OOB *demand_OOB; // Page level OOB
D_SRAM *d_sram; // SRAM for contain block data temporarily

int32_t DPA_status; // Data page allocation
int32_t TPA_status; // Translation page allocation
int32_t PBA_status; // Block allocation
int8_t needGC; // Indicates need of GC
int32_t tpage_onram_num;

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
		d_sram[i].OOB_RAM = (D_OOB){-1, 0, 0};
	}
	for(int i = 0; i < _NOP; i++)
		demand_OOB[i] = (D_OOB){-1, 0, 0};
#endif

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
}

void demand_destroy(lower_info *li, algorithm *algo){
	free(CMT);
	free(GTD);
	free(demand_OOB);
	free(d_sram);
	while(head)
		queue_delete(head);
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
