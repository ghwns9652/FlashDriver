#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "normal.h"
#include "../../interface/interface.h"
#include "../../bench/bench.h"

#define LOWERTYPE 10

extern MeasureTime mt;
struct algorithm __normal={
	.create=normal_create,
	.destroy=normal_destroy,
	.read=normal_get,
	.write=normal_set,
	.remove=normal_remove,
	.iter_create=NULL,
	.iter_next=NULL,
	.iter_release=NULL,
	.multi_set=NULL,
	.range_get=NULL
};

n_cdf _cdf[LOWERTYPE];

char temp[PAGESIZE];

int find_key,miss_key;
//int number_of_get_req;
int max_try=0;
void normal_cdf_print(){

}
uint32_t normal_create (lower_info* li,algorithm *algo){
	algo->li=li;
	memset(temp,'x',PAGESIZE);
	for(int i=0; i<LOWERTYPE; i++){
		_cdf[i].min=UINT_MAX;
	}
	return 1;
}
void normal_destroy (lower_info* li, algorithm *algo){
	normal_cdf_print();
	printf("%d,%d\n%d(find_key,miss_key,max_try)\n",find_key,miss_key,max_try);
	return;
}

int normal_cnt;

int hash(int key){
	return key%(_NOP/2);
}


uint32_t normal_get(request *const req){
	bench_algo_start(req);
	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	req->type=FS_GET_T;
	//printf("Normal Get In\n");

	normal_params *params;
	if(req->params==NULL){	
		params=(normal_params*)malloc(sizeof(normal_params));
		params->cnt=0;
		req->params=(void*)params;
		//req->params=params;
		//printf("params is NULL\n");
	}
	else{
		params=(normal_params*)req->params;
		printf("params is update\n");
	}

	int cnt = params->cnt;
	int hash_key = hash(req->key) + cnt*cnt + cnt;
	hash_key %= _NOP;

	__normal.li->read(hash_key,PAGESIZE,req->value,req->isAsync,my_req);
	normal_cnt++;	
	return 1;	
}
uint32_t normal_set(request *const req){
	if(req->key==61402){
		printf("print\n");
	}
	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;

	req->type=FS_SET_T;

	normal_params *params;
	if(req->params==NULL){	
		params=(normal_params*)malloc(sizeof(normal_params));
		params->cnt=0;
		params->finding=0;
		req->params=(void*)params;
		//req->params=params;
	}
	else{
		params=(normal_params*)req->params;
	}

	int cnt= params->cnt;
	int hash_key = hash(req->key) + cnt*cnt + cnt;
	hash_key %= _NOP;

	switch (params->finding){
		case 3:		//change ppa
		case 0:
			my_req->type=DATAR;
			__normal.li->read(hash_key,PAGESIZE,req->value,req->isAsync,my_req);
			break;
		case 2:		//change ppa and write
		case 1:		//write
			memcpy(req->value->value,&req->key,sizeof(req->key));
			my_req->type=DATAW;
			__normal.li->write(hash_key,PAGESIZE,req->value,req->isAsync,my_req);
			break;

	}
	// 	memcp(req->value->value,&req->key,sizeof(req->key));
	normal_cnt++;
	return 0;
}
uint32_t normal_remove(request *const req){
	__normal.li->trim_block(req->key,NULL);
	return 1;
}
void *normal_end_req(algo_req* input){
	request *res=input->parents;
	normal_params *params;
	params=(normal_params*)res->params;
	if (res->type==FS_GET_T){
		KEYT temp_key;
		memcpy(&temp_key,res->value->value,sizeof(KEYT));
		if (res->key==temp_key){
			find_key++;			///////////////////////////////////////////findkey++////////////
			free(input);
			free(params);
		}else{
			free(input);
			if(max_try>=params->cnt){
				params->cnt++;
				inf_assign_try(res);
				return NULL;
			}
			else{
				miss_key++;			////////////////////////////////misskey++////////////////
			}
		}
	}else if(input->type==DATAR){		//set,read
		KEYT temp_key;
		memcpy(&temp_key,res->value->value,sizeof(KEYT));
		if(temp_key==0){	//not found
			params->finding = 1;
		}else if (res->key==temp_key){
			params->finding = 2;
		}else{
			params->finding = 3;
			params->cnt++;
		}
		free(input);
		inf_assign_try(res);
		return NULL;
	}else{		//set,write
		if(max_try<params->cnt)
			max_try=params->cnt;
		free(input);
		free(params);
	}
	res->end_req(res);
}
