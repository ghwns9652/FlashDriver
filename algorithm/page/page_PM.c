#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "page.h"
#include "../../bench/bench.h"
#include "PM_operation.h"

extern int64_t local_page_position;
extern int64_t block_position;
extern int64_t reserved_local;
extern int64_t reserved_block;
extern int8_t GC_phase;

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
uint16_t *invalid_per_block;

BINFO* binfo[_NOB];

uint32_t pbase_create(lower_info* li, algorithm *algo) //define & initialize mapping table.
{
	algo->li = li; 	//allocate li parameter to algorithm's li.
	page_TABLE = (TABLE*)malloc(sizeof(TABLE)*_NOP);
	for(int i = 0; i < _NOP; i++)
	{
     	page_TABLE[i].lpa_to_ppa = -1;
		page_TABLE[i].valid_checker = 0;
	}
	
	page_OOB = (OOB *)malloc(sizeof(OOB)*_NOP);
	for (int i = 0; i < _NOP; i++)
	{
		page_OOB[i].reverse_table = -1;
	}	
	page_SRAM = (SRAM*)malloc(sizeof(SRAM)*_PPB);
	for (int i = 0; i<_PPB; i++)
	{
		page_SRAM[i].lpa_RAM = -1;
		page_SRAM[i].VPTR_RAM = NULL;
	}

	invalid_per_block = (uint16_t*)malloc(sizeof(uint16_t)*_NOB);
	for (int i = 0; i<_NOB; i++)
	{
		invalid_per_block[i] = 0;
	}
	
	for (int i = 0; i < _NOB; i++)
	{
		binfo[i] = (BINFO*)malloc(sizeof(BINFO));
		binfo[i]->head_ppa = i * _PPB;
		binfo[i]->BAD = 0;
	}

	BINFO** bp = binfo;
	Selector_Init(bp);//init selector with fake bp.
	//init mapping table.
}	//now we can use page table after pbase_create operation.



void pbase_destroy(lower_info* li, algorithm *algo)
{					  
        free(page_OOB);
        free(invalid_per_block);
		  free(page_SRAM);
		  free(page_TABLE);
		  Selector_Destroy();
		  for (int i = 0; i < _NOB; i++)
		  {
			  free(binfo[i]);
		  }
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
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req)); //init reqeust
	my_req->parents = req;
	my_req->end_req=pbase_end_req;//allocate end_req for request.
	KEYT target = page_TABLE[req->key].lpa_to_ppa;
	bench_algo_end(req);
	if (page_TABLE[req->key].lpa_to_ppa == -1)
	{
		printf("read an empty page!");
		return;
	}
	algo_pbase.li->pull_data(target,PAGESIZE,req->value,0,my_req,0);
	//key-value operation.
	//Question: why value type is char*?
}

uint32_t pbase_set(request* const req)
{
	if ((PPA_status/_PPB) < 10) 
	{
		printf("local page in block :%d\n",local_page_position);
		printf("block number : %d\n",PPA_status/_PPB);
	}
	bench_algo_start(req);
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = pbase_end_req;

	//garbage_collection necessity detection.
	if ((IsEmpty(&empty_queue))&&(local_page_position == _PPB))
	{
		pbase_garbage_collection();
	}

	//!garbage_collection.
	
	if (page_TABLE[req->key].lpa_to_ppa != -1)
	{
		int temp = page_TABLE[req->key].lpa_to_ppa; //find old ppa.
		page_TABLE[temp].valid_checker = 0; //set that ppa validity to 0.
		invalid_per_block[temp/_PPB] += 1;
	}
	PPA_status = Giveme_Page(0);
	page_TABLE[req->key].lpa_to_ppa = PPA_status; //map ppa status to table.
	page_TABLE[PPA_status].valid_checker = 1; 
	page_OOB[PPA_status].reverse_table = req->key;//reverse-mapping.
	KEYT set_target = PPA_status;
	bench_algo_end(req);
	algo_pbase.li->push_data(set_target,PAGESIZE,req->value,0,my_req,0);
}

uint32_t pbase_remove(request* const req)
{
	page_TABLE[req->key].lpa_to_ppa = -1; //reset to default.
	page_OOB[req->key].reverse_table = -1; //reset reverse_table to default.
}

uint32_t SRAM_load(int ppa, int a)
{
	value_set* value_PTR;
	value_PTR =(value_set*)malloc(sizeof(value_PTR));
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = NULL;
	my_req->end_req = pbase_algo_end_req; //request termination.
	algo_pbase.li->pull_data(ppa,PAGESIZE,value_PTR,0,my_req,0);
	page_SRAM[a].lpa_RAM = page_OOB[ppa].reverse_table;//load reverse-mapped lpa.
	page_SRAM[a].VPTR_RAM = value_PTR;
	
}

uint32_t SRAM_unload(int ppa, int a)
{
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->end_req = pbase_algo_end_req;
	my_req->parents = NULL;
	algo_pbase.li->push_data(ppa,PAGESIZE,page_SRAM[a].VPTR_RAM,0,my_req,0);
	
	page_TABLE[page_SRAM[a].lpa_RAM].lpa_to_ppa = ppa;
	page_TABLE[ppa].valid_checker = 1;
	page_OOB[ppa].reverse_table = page_SRAM[a].lpa_RAM;
	
	page_SRAM[a].lpa_RAM = -1;
	free(page_SRAM[a].VPTR_RAM);
	page_SRAM[a].VPTR_RAM = NULL;
}

uint32_t pbase_garbage_collection()//do pbase_read and pbase_set 
{
	GC_phase = 1;
	printf("enterged GC..!");
	int target_block = 0;
	int invalid_num = 0;
	for (int i = 0; i < _NOB; i++)
	{
		if(invalid_per_block[i] >= invalid_num)
		{
			target_block = i;
			invalid_num = invalid_per_block[i];
		}
	}//found block with the most invalid block.
	PPA_status = target_block* _PPB; //GC target
	int valid_component = _PPB - invalid_num;
	int a = 0;
	for (int i = 0; i < _PPB; i++)
	{
		if (page_TABLE[PPA_status + i].valid_checker == 1)
		{
			SRAM_load(PPA_status + i, a);
			a++;
			page_TABLE[PPA_status + i].valid_checker = 0;
		}
	}
	algo_pbase.li->trim_block(PPA_status, false);//trim target block

	for (int i = 0; i<valid_component; i++)
	{
		SRAM_unload(rsv_ppa,i);
		rsv_ppa++;
	}

	invalid_per_block[target_block] = 0;

	BINFO* rsv_trans;//rsv is transformed to empty_queue
	rsv_trans = (BINFO*)malloc(sizeof(BINFO));
	rsv_trans->head_ppa = rsv_ppa - valid_component;
	rsv_trans->BAD = 0;
	Enqueue(&empty_queue, rsv_trans);
	local_page_position = valid_component;

	rsv_ppa = PPA_status;//GC target became new reserved.
}
