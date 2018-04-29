#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "page.h"

extern int64_t local_page_position;
extern int64_t block_position;
extern int64_t reserved_local;
extern int64_t reserved_block;
extern int8_t GC_phase;
//extern from Page Manager.

extern Block* blockArray;
extern nV_T** numValid_map;
//extern from Block Manager.

extern B_queue empty_queue;
extern B_queue reserved_queue;


struct algorithm algo_pbase=
{
	.create = pbase_create,
	.destroy = pbase_destroy,
	.get = pbase_get,
	.set = pbase_set,
	.remove = pbase_remove
};

uint32_t PPA_status = 0;//GC target PPA.
int init_done = 0;//check if initial write is done.
int rsv_ppa = 0;//first ppa of reserved area.

TABLE *page_TABLE;
OOB *page_OOB;
SRAM *page_SRAM;

uint32_t pbase_create(lower_info* li, algorithm *algo) //define & initialize mapping table.
{
	PM_Init();//initialize page_manager and block_manager.
	algo->li = li; 	//allocate li parameter to algorithm's li.
	page_TABLE = (TABLE*)malloc(sizeof(TABLE)*_NOP);
	for(int i = 0; i < _NOP; i++)
	{
     	page_TABLE[i].lpa_to_ppa = -1;
	}//mapping table
	
	page_OOB = (OOB *)malloc(sizeof(OOB)*_NOP);
	for (int i = 0; i < _NOP; i++)
	{
		page_OOB[i].reverse_table = -1;
	}//OOB util.	

	page_SRAM = (SRAM*)malloc(sizeof(SRAM)*_PPB);
	for (int i = 0; i<_PPB; i++)
	{
		page_SRAM[i].lpa_RAM = -1;
		page_SRAM[i].VPTR_RAM = NULL;
	}//For GC swap.

}	//now we can use page table after pbase_create operation.

void pbase_destroy(lower_info* li, algorithm *algo)
{					  
    free(page_OOB);
	free(page_SRAM);
	free(page_TABLE);
	PM_Destroy();
}

void *pbase_end_req(algo_req* input)
{
	request *res=input->parents;
	res->end_req(res);
	free(input);
}

void *pbase_algo_end_req(algo_req* input)
{
	free(input);
}

uint32_t pbase_get(request* const req)
{
	//put request in normal_param first.
	//request has a type, key and value.

	bench_algo_start(req);
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));//init reqeust
	my_req->parents = req;
	my_req->end_req=pbase_end_req;//allocate end_req for request.
	KEYT target = page_TABLE[req->key].lpa_to_ppa;
	bench_algo_end(req);
	if (page_TABLE[req->key].lpa_to_ppa == -1)
	{
		printf("read an empty page!");
	}
	algo_pbase.li->pull_data(target,PAGESIZE,req->value,0,my_req);
	//key-value operation.
}

uint32_t pbase_set(request* const req)
{
	bench_algo_start(req);
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = pbase_end_req;

	//garbage_collection necessity detection.
	if ((IsEmpty(&empty_queue))&&(local_page_position == _PPB))
	{	pbase_garbage_collection(); }
	//!garbage_collection.
	
	if (page_TABLE[req->key].lpa_to_ppa != -1)//already mapped case.
	{
		int temp = page_TABLE[req->key].lpa_to_ppa; //find old ppa.
		BM_invalidate_ppa(blockArray, temp);
	}

	PPA_status = Alloc_Page();//get available physical page addr.

	page_TABLE[req->key].lpa_to_ppa = PPA_status; //map on table. 
	page_OOB[PPA_status].reverse_table = req->key;//reverse-mapping.
	KEYT set_target = PPA_status; //target set.
	BM_update_block_with_push(blockArray, PPA_status);//update blockArray.
	bench_algo_end(req);
	algo_pbase.li->push_data(set_target,PAGESIZE,req->value,0,my_req);
}

uint32_t pbase_remove(request* const req)
{
	page_TABLE[req->key].lpa_to_ppa = -1; //reset to default.
	page_OOB[req->key].reverse_table = -1; //reset reverse_table to default.
}

uint32_t SRAM_load(int ppa, int a)
{
	value_set* value_PTR; //make new value_set.
	value_PTR =inf_get_valueset(NULL, 1, PAGESIZE);//set valueset as read mode.
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = NULL;
	my_req->end_req = pbase_algo_end_req; //request termination.
	algo_pbase.li->pull_data(ppa,PAGESIZE,value_PTR,0,my_req);
	page_SRAM[a].lpa_RAM = page_OOB[ppa].reverse_table;//load reverse-mapped lpa.
	page_SRAM[a].VPTR_RAM = value_PTR;
	
}

uint32_t SRAM_unload(int ppa, int a)
{
	value_set *value_PTR; //make new value_set.
	value_PTR = inf_get_valueset(page_SRAM[a].VPTR_RAM->value, 2,PAGESIZE);//set valueset as write mode.
	
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->end_req = pbase_algo_end_req;
	my_req->parents = NULL;
	algo_pbase.li->push_data(ppa,PAGESIZE,page_SRAM[a].VPTR_RAM,0,my_req);
	
	page_TABLE[page_SRAM[a].lpa_RAM].lpa_to_ppa = ppa; //mapping table update.
	blockArray[ppa/_PPB].ValidP[ppa/_PPB] = 1;
	page_OOB[ppa].reverse_table = page_SRAM[a].lpa_RAM;
	
	inf_free_valueset(value_PTR, 2);
	inf_free_valueset(page_SRAM[a].VPTR_RAM, 1);

	page_SRAM[a].lpa_RAM = -1;
	page_SRAM[a].VPTR_RAM = NULL;
}

uint32_t pbase_garbage_collection()//do pbase_read and pbase_set 
{
	GC_phase = 1;
	printf("enterged GC..");
	int target_block = 0;
	int valid_comp = 0;
	target_block = BM_get_gc_victim(blockArray,numValid_map);
	PPA_status = target_block* _PPB; //GC target.
	valid_comp = blockArray[target_block].numValid;
	int a = 0;
	for (int i = 0; i < _PPB; i++)
	{
		if (blockArray[target_block].ValidP[i] == 1)
		{
			SRAM_load(PPA_status + i, a);
			a++;
		}
	}//load valid components on RAM.

	BM_update_block_with_gc(blockArray, PPA_status);
	BM_update_block_with_trim(blockArray, PPA_status);//give info to Manager.
	algo_pbase.li->trim_block(PPA_status, false);//trim target block.

	for (int i = 0; i<valid_comp; i++)
	{
		SRAM_unload(rsv_ppa,i);
		rsv_ppa++;
		BM_update_block_with_push(blockArray,rsv_ppa);
	}//valid data is on reserved block now.
	Dequeue(&reserved_queue);	
	//switch reserved block and trimmed block.
	Block* new_emp = &blockArray[rsv_ppa/_PPB];//reserved block is transferred to empty_queue
	Block* new_rsv = &blockArray[target_block];//emptied block is transferred to reserved_queue. 
	Enqueue(&empty_queue, new_emp);
	Enqueue(&reserved_queue, new_rsv);

	rsv_ppa = new_rsv->PBA * _PPB;//GC target became new reserved.
}
