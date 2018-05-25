#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "page.h"

struct algorithm algo_pbase=
{
	.create = pbase_create,
	.destroy = pbase_destroy,
	.get = pbase_get,
	.set = pbase_set,
	.remove = pbase_remove
};

uint32_t PPA_status = 0;
int init_done = 0;//check if initial write is done.

TABLE *page_TABLE;
OOB *page_OOB;
SRAM *page_SRAM;
uint16_t *invalid_per_block;


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
	printf("pbase_create done!\n");

	//init mapping table.
}	//now we can use page table after pbase_create operation.



void pbase_destroy(lower_info* li, algorithm *algo)
{					  
	free(page_OOB);
	free(invalid_per_block);
	free(page_SRAM);
	free(page_TABLE);
}

void *pbase_end_req(algo_req* input)
{
	request *res=input->parents;
	res->end_req(res);//call end_req of parent req.
	free(input); //free target algo_req.
}

void *pbase_algo_end_req(algo_req* input)
{
	free(input);
}//there may be warning.

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
	if (target == -1)
	{
		printf("\nNO data.\n");
		pbase_end_req(my_req);
		return 0;
	}
	else
	{
	//	algo_pbase.li->pull_data(target,PAGESIZE,req->value,0,my_req);
		pbase_end_req(my_req);
		printf("\n==== get data ===\n");
		printf("target is : %d\n",req->key);
		printf("assigned ppa is %d\n",target);
		printf("==== get done ===\n");
	}
	//key-value operation.
}

uint32_t pbase_set(request* const req)
{
	bench_algo_start(req);
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = pbase_end_req;

	//garbage_collection necessity detection.
	if (PPA_status == _NOP)
	{
		pbase_garbage_collection();
		init_done = 1;
	}

	else if ((init_done == 1) && (PPA_status % _PPB == 0))
	{
		pbase_garbage_collection();
	}
	//!garbage_collection.
	
	if (page_TABLE[req->key].lpa_to_ppa != -1)
	{
		int temp = page_TABLE[req->key].lpa_to_ppa; //find old ppa.
		page_TABLE[temp].valid_checker = 0; //set that ppa validity to 0.
		invalid_per_block[temp/_PPB] += 1;
		printf("phys page %d is invalid.\n",temp);
	}
	
	page_TABLE[req->key].lpa_to_ppa = PPA_status; //map ppa status to table.
	page_TABLE[PPA_status].valid_checker = 1; 
	page_OOB[PPA_status].reverse_table = req->key;//reverse-mapping.
	KEYT set_target = PPA_status;
	PPA_status++;
	bench_algo_end(req);	
	
	printf("\n==== set data ===\n");
	printf("target is : %d\n",req->key);
	printf("value is : %c\n",req->value->value[0]);
	printf("==== set done ===\n");

	algo_pbase.li->push_data(set_target,PAGESIZE,req->value,0,my_req);

}

uint32_t pbase_remove(request* const req)
{
	page_TABLE[req->key].lpa_to_ppa = -1; //reset to default.
	page_OOB[req->key].reverse_table = -1; //reset reverse_table to default.
}

uint32_t SRAM_load(int ppa, int a)
{
	value_set* value_PTR ; //make new value_set
	value_PTR = inf_get_valueset(NULL,1,PAGESIZE);
	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = NULL;
	my_req->end_req = pbase_algo_end_req; //request termination.
	algo_pbase.li->pull_data(ppa,PAGESIZE,value_PTR,0,my_req);
	sleep(1);
	printf("\n===RAMLOAD===\n");
	printf("loaded item : %c\n",value_PTR->value[0]);
	printf("===RAM_END===\n");
	page_SRAM[a].lpa_RAM = page_OOB[ppa].reverse_table;//load reverse-mapped lpa.
	page_SRAM[a].VPTR_RAM = value_PTR;
}

uint32_t SRAM_unload(int ppa, int a)
{
	value_set *value_PTR;
	value_PTR = inf_get_valueset(page_SRAM[a].VPTR_RAM->value,2,PAGESIZE);//set valueset as write mode.

	algo_req * my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->end_req = pbase_algo_end_req;
	my_req->parents = NULL;
	algo_pbase.li->push_data(ppa,PAGESIZE,page_SRAM[a].VPTR_RAM,0,my_req);
   printf("\npushed data is %c\n",page_SRAM[a].VPTR_RAM->value[0]);	
	page_TABLE[page_SRAM[a].lpa_RAM].lpa_to_ppa = ppa;
	page_TABLE[ppa].valid_checker = 1;
	page_OOB[ppa].reverse_table = page_SRAM[a].lpa_RAM;
   inf_free_valueset(page_SRAM[a].VPTR_RAM,1);	
	inf_free_valueset(value_PTR,2);
	page_SRAM[a].lpa_RAM = -1;
	page_SRAM[a].VPTR_RAM = NULL;
	sleep(1);
}

uint32_t pbase_garbage_collection()//do pbase_read and pbase_set 
{
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
	}//find block with the most invalid block.
	PPA_status = target_block* _PPB;
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
	algo_pbase.li->trim_block(PPA_status, false);

	for (int i = 0; i<valid_component; i++)
	{
		SRAM_unload(PPA_status,i);
		PPA_status++;
	}

	invalid_per_block[target_block] = 0;
}
