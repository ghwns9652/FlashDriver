#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "../include/lsm_settings.h"
#include "../include/FS.h"
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "interface.h"

#include "../include/kuk_socket_lib/kuk_sock.h"
#ifdef Lsmtree
int skiplist_hit;
#endif
kuk_sock *net_worker;
#define IP "127.0.0.1"
#define PORT 8888
#define REQSIZE (sizeof(uint64_t)*2+sizeof(uint8_t))
#define PACKETSIZE 4096
bool force_write_start;

void *flash_ack2clnt(void *param){
	uint8_t type=*((uint8_t*)param);
	switch(type){
		case FS_NOTFOUND_T:
		case FS_GET_T:
			kuk_ack2clnt(net_worker);
			break;
		default:
			break;
	}
	free(param);
	return NULL;
}
void *flash_ad(kuk_sock* ks){
	uint8_t type=*((uint8_t*)ks->p_data[0]);
	uint64_t key=*((uint64_t*)ks->p_data[1]);
	uint64_t len=*((uint64_t*)ks->p_data[2]);
		
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);

	value_set temp;
	temp.value=t_value;
	temp.dmatag=-1;
	temp.length=(uint32_t)len;
	static int cnt=0;
	printf("make cnt:%d\n",cnt++);
	inf_make_req_special(type,(uint32_t)key,&temp,0,flash_ack2clnt);
	return NULL;
}
void *flash_decoder(kuk_sock *ks, void*(*ad)(kuk_sock*)){
	char **parse=ks->p_data;
	if(parse==NULL){
		parse=(char**)malloc(3*sizeof(char*)+1);
		parse[0]=(char*)malloc(sizeof(uint8_t));//type
		parse[1]=(char*)malloc(sizeof(uint32_t));//key
		parse[2]=(char*)malloc(sizeof(uint32_t));//length
		parse[3]=NULL;
		ks->p_data=parse;
	}

	char *dd=&ks->data[ks->data_idx];
	memcpy(parse[0],&dd[0],sizeof(uint8_t));
	memcpy(parse[1],&dd[sizeof(uint8_t)],sizeof(uint64_t));
	memcpy(parse[2],&dd[REQSIZE-sizeof(uint64_t)],sizeof(uint64_t));
	
	if((*(int*)parse[0])==ENDFLAG){
		return NULL;
	}
	ks->data_idx+=REQSIZE;
	ad(ks);
	return (void*)parse;
}
int main(int argc,char* argv[]){
	inf_init();
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	bench_init(1);
	bench_add(NOR,0,-1,-1);
	
	/*network initialize*/
	net_worker=kuk_sock_init((PACKETSIZE/REQSIZE)*REQSIZE,flash_decoder,flash_ad);
	kuk_open(net_worker,IP,PORT);
	kuk_bind(net_worker);
	kuk_listen(net_worker,5);
	kuk_accept(net_worker);
	//while(kuk_service(net_worker,1)){}
	uint32_t len=0;
	while((len=kuk_recv(net_worker,net_worker->data,net_worker->data_size))){
		net_worker->data_idx=0;
		while(len!=net_worker->data_idx){
			net_worker->decoder(net_worker,net_worker->after_decode);
		}
	}
	
	kuk_sock_destroy(net_worker);
	force_write_start=true;

	bench_print();
	bench_free();
	return 0;
}
