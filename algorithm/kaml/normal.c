#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "normal.h"
#include "sha256.h"
#include "../../interface/interface.h"
#include "../../bench/bench.h"
#include "pftl.h"

#define LOWERTYPE 10

extern MeasureTime mt;
struct algorithm algo_kaml={
    .create=normal_create,
    .destroy=normal_destroy,
    .read=normal_get,
    .write=normal_set,
    .remove=normal_remove,
    .iter_create=NULL,
    .iter_next=NULL,
    .iter_next_with_value=NULL,
    .iter_release=NULL,
    .multi_set=NULL,
    .range_get=NULL
};

n_cdf _cdf[LOWERTYPE];

char temp[PAGESIZE];

int find_key,miss_key;
//int number_of_get_req;
int max_try=0;

static int collision;
static int _write;
static int update;
static int read_for_write;
static int else_find_key;
static int total;

void normal_cdf_print(){
}
uint32_t normal_create (lower_info* li,algorithm *algo){
//    algo->li=li;
	pftl_create(li, algo);
	memset(temp,'x',PAGESIZE);
	for(int i=0; i<LOWERTYPE; i++){
		 _cdf[i].min=UINT_MAX;
	}
	return 1;
}
void normal_destroy (lower_info* li, algorithm *algo){
    normal_cdf_print();
    printf("%d,%d(find_key,miss_key)\n%d(max_try)\n%d, %d(write,update)\n%d(collision)\n%d(read for write)\n%d(else_find_key)\n%d(collision request)",find_key,miss_key,max_try,_write,update,collision,read_for_write,else_find_key,total);
    return;
}

int normal_cnt;
/*
   int hash(int key){
   return key%(_NOP/2);
   }
   */

uint32_t hashing_key(char* key,uint8_t len) {
    char* string;
    Sha256Context ctx;
    SHA256_HASH hash;
    int bytes_arr[8];
    uint32_t hashkey;

    string = key;

    Sha256Initialise(&ctx);
    Sha256Update(&ctx, (unsigned char*)string, len);
    Sha256Finalise(&ctx, &hash);

    for(int i=0; i<8; i++) {
        bytes_arr[i] = ((hash.bytes[i*4] << 24) | (hash.bytes[i*4+1] << 16) | \
                (hash.bytes[i*4+2] << 8) | (hash.bytes[i*4+3]));
    }

    hashkey = bytes_arr[0];
    for(int i=1; i<8; i++) {
        hashkey ^= bytes_arr[i];
    }

    return hashkey;
}

uint32_t normal_get(request *const req){
    bench_algo_start(req);
    hash_req *my_req=(hash_req*)malloc(sizeof(hash_req));
    my_req->parents=req;
    my_req->end_req=normal_end_req;
    req->type=FS_GET_T;
    //printf("Normal Get In\n");

    normal_params *params;
    if(req->params==NULL){	
        params=(normal_params*)malloc(sizeof(normal_params));
        params->cnt=0;
        req->params=(void*)params;
        params->temp=0;
        //req->params=params;
        //printf("params is NULL\n");
    }
    else{
        params=(normal_params*)req->params;
        //printf("params is update\n");
    }

    int cnt = params->cnt;
    //int hash_key = hash(req->key) + cnt*cnt + cnt;
    uint32_t hash_key = hashing_key(req->key.key,req->key.len) + cnt*cnt + cnt;
    hash_key %= _NOP;

    my_req->hash_key = hash_key;
    pftl_read(my_req);
    normal_cnt++;
    return 1;	
}
uint32_t normal_set(request *const req){
    hash_req *my_req=(hash_req*)malloc(sizeof(hash_req));
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

    //printf("normal_set %.*s\n",KEYFORMAT(req->key));
    int cnt= params->cnt;
    uint32_t hash_key = hashing_key(req->key.key,req->key.len) + cnt*cnt + cnt;
    hash_key %= _NOP;
    my_req->hash_key = hash_key;
    if(!exist(hash_key)){
        _write++;
        memcpy(req->value->value, &req->key.len, sizeof(req->key.len));
        memcpy(&(req->value->value[1]), req->key.key, req->key.len);
        my_req->type=DATAW;
        pftl_write(my_req);
    }else{
        switch (params->finding){
            case 3:
                collision++;        //change ppa
            case 0:
                my_req->type=DATAR;
                pftl_read(my_req);
                break;
            case 2:		//change ppa and write
                update++;
//            case 1:		//write
                _write++;
                memcpy(req->value->value, &req->key.len, sizeof(req->key.len));
                memcpy(&(req->value->value[1]), req->key.key, req->key.len);
                my_req->type=DATAW;
                pftl_write(my_req);
                break;

        }
    }
    normal_cnt++;
    return 0;

}
///////////////////////////////////////////////////////////////////////////////////*********///
uint32_t normal_remove(request *const req){
    normal_params *params;
    params=(normal_params*)req->params;
    int cnt= params->cnt;
    uint32_t hash_key = hashing_key(req->key.key,req->key.len) + cnt*cnt + cnt;
    hash_key %= _NOP;

    algo_kaml.li->trim_block(hash_key,NULL);
    return 1;
}
void *normal_end_req(hash_req* input){
    request *res=input->parents;
    normal_params *params;
    params=(normal_params*)res->params;
    if (res->type==FS_GET_T){
        KEYT temp_key;
        //printf("%d,%d(len, length)\n",res->value->len,res->value->length);
        temp_key.len = *((uint8_t*)res->value->value);
        temp_key.key = (char*)malloc(temp_key.len);
        memcpy(temp_key.key,(res->value->value+1),temp_key.len);
        if (KEYCMP(res->key,temp_key)==0){
            find_key++;			///////////////////////////////////////////findkey++////////////
            if(params->temp==1){
                total++;
            }
            free(input);
            free(params);
        }else{
            params->temp=1;
            else_find_key++;
            free(input);
            if(max_try>=params->cnt){
                params->cnt++;
                inf_assign_try(res);
                free(temp_key.key);
                return NULL;
            }
            else{
                miss_key++;			////////////////////////////////misskey++////////////////
            }
        }
        free(temp_key.key);
    }else if(input->type==DATAR){		//set,read
        read_for_write++;
        KEYT temp_key;
        temp_key.len = *((uint8_t*)res->value->value);
        temp_key.key = (char*)malloc(temp_key.len);
        memcpy(temp_key.key,(res->value->value+1),temp_key.len);
        /*  
        if(temp_key.len==0){	//not found
            params->finding = 1;
        
        }else*/
        if (KEYCMP(res->key,temp_key)==0){
            params->finding = 2;
        }else{
            params->finding = 3;
            params->cnt++;
        }
        free(input);
        inf_assign_try(res);
        free(temp_key.key);
        return NULL;
    }else{		//set,write
        if(max_try<params->cnt)
            max_try=params->cnt;
        free(input);
        free(params);
    }
    res->end_req(res);
    return NULL;
}
