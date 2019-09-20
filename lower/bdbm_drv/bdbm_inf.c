#include "../../include/settings.h"
#include "../../bench/bench.h"
#include "../../bench/measurement.h"
#include "../../include/container.h"
#include "frontend/libmemio/libmemio.h"
#include "bdbm_inf.h"
#include "../../interface/bb_checker.h"
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
pthread_mutex_t test_lock;

memio_t *mio;
lower_info memio_info={
	.create=memio_info_create,
	.destroy=memio_info_destroy,
	.write=memio_info_push_data,
	.read=memio_info_pull_data,
	.device_badblock_checker=memio_badblock_checker,
	.trim_block=memio_info_trim_block,
	.refresh=memio_info_refresh,
	.stop=memio_info_stop,
	.lower_alloc=memio_alloc_dma,
	.lower_free=memio_free_dma,
	.lower_flying_req_wait=memio_flying_req_wait,
	.lower_show_info=memio_show_info_
};

uint32_t memio_info_create(lower_info *li){
	li->NOB=_NOB;
	li->NOP=_NOP;
	li->SOB=BLOCKSIZE;
	li->SOP=PAGESIZE;
	li->SOK=sizeof(uint32_t);
	li->PPB=_PPB;
	li->TS=TOTALSIZE;

	li->write_op=li->read_op=li->trim_op=0;
	pthread_mutex_init(&memio_info.lower_lock,NULL);
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
	pthread_mutex_init(&test_lock, 0);
	pthread_mutex_lock(&test_lock);
	
	memset(li->req_type_cnt,0,sizeof(li->req_type_cnt));
	mio=memio_open();
	
	return 1;
}

static uint8_t test_type(uint8_t type){
	uint8_t t_type=0xff>>1;
	return type&t_type;
}
void *memio_info_destroy(lower_info *li){
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
	for(int i=0; i<LREQ_TYPE_NUM;i++){
		fprintf(stderr,"%s %lu\n",bench_lower_type(i),li->req_type_cnt[i]);
	}

    fprintf(stderr,"Total Read Traffic : %lu\n", li->req_type_cnt[1]+li->req_type_cnt[3]+li->req_type_cnt[5]+li->req_type_cnt[7]);
    fprintf(stderr,"Total Write Traffic: %lu\n\n", li->req_type_cnt[2]+li->req_type_cnt[4]+li->req_type_cnt[6]+li->req_type_cnt[8]);
    fprintf(stderr,"Total WAF: %.2f\n\n", (float)(li->req_type_cnt[2]+li->req_type_cnt[4]+li->req_type_cnt[6]+li->req_type_cnt[8]) / li->req_type_cnt[6]);

	li->write_op=li->read_op=li->trim_op=0;
	memio_close(mio);

	return NULL;
}

void *memio_info_push_data(uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req *const req){
	if(value->dmatag==-1){
		printf("dmatag -1 error!\n");
		exit(1);
	}
	
	uint8_t t_type=test_type(req->type);
	if(t_type < LREQ_TYPE_NUM){
		memio_info.req_type_cnt[t_type]++;
	}
	memio_write(mio,bb_checker_fix_ppa(ppa),(uint32_t)size,(uint8_t*)value->value,async,(void*)req,value->dmatag);
	//memio_write(mio,ppa,(uint32_t)size,(uint8_t*)value->value,async,(void*)req,value->dmatag);
	//pthread_mutex_lock(&test_lock);
	return NULL;
}

void *memio_info_pull_data(uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req *const req){
	if(value->dmatag==-1){
		printf("dmatag -1 error!\n");
		exit(1);
	}
	uint8_t t_type=test_type(req->type);
	if(t_type < LREQ_TYPE_NUM){
		memio_info.req_type_cnt[t_type]++;
	}
	memio_read(mio,bb_checker_fix_ppa(ppa),(uint32_t)size,(uint8_t*)value->value,async,(void*)req,value->dmatag);
	//memio_read(mio,ppa,(uint32_t)size,(uint8_t*)value->value,async,(void*)req,value->dmatag);
	//pthread_mutex_lock(&test_lock);
	return NULL;
}

void *memio_info_trim_block(uint32_t ppa, bool async){
	//int value=memio_trim(mio,bb_checker_fix_ppa(ppa),(1<<14)*PAGESIZE,NULL);
	int value=memio_trim(mio,bb_checker_fixed_segment(ppa),(1<<14)*PAGESIZE,NULL);
	value=memio_trim(mio,bb_checker_paired_segment(ppa),(1<<14)*PAGESIZE,NULL);
	
	memio_info.req_type_cnt[TRIM]++;
	if(value==0){
		return (void*)-1;
	}
	else
		return NULL;
}

void *memio_info_refresh(struct lower_info* li){
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
	li->write_op=li->read_op=li->trim_op=0;
	return NULL;
}
void *memio_badblock_checker(uint32_t ppa,uint32_t size, void*(*process)(uint64_t,uint8_t)){
	memio_trim(mio,ppa,size,process);
	return NULL;
}

void memio_info_stop(){}

void memio_flying_req_wait(){
	while(!memio_is_clean(mio));
}
void memio_show_info_(){
	memio_show_info();
}
