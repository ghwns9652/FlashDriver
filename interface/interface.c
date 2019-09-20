#include "interface.h"
#include "queue.h"
#include "../include/container.h"

#include "../include/FS.h"
#include "../bench/bench.h"
#include "../bench/measurement.h"

#ifdef KVSSD
#include "../include/data_struct/hash_kv.h"
#else
#include "../include/data_struct/hash.h"
#endif

#include "../include/data_struct/redblack.h"
#include "../include/utils/cond_lock.h"
#include "bb_checker.h"
#include "buse.h"
#include "layer_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

extern Redblack rb_tree;
extern pthread_mutex_t rb_lock;

#ifdef bdbm_drv
extern struct lower_info memio_info;
#endif
extern struct lower_info aio_info;
#ifdef network
extern struct lower_info net_info;
#endif
#ifdef vcu108
extern struct lower_info vcu_info;
#endif

extern master *_master;

MeasureTime mt;
MeasureTime mt4;
master_processor mp;

bool sync_apps;
void *p_main(void*);
cl_lock *flying,*inf_cond;

#ifdef BUSE_MEASURE
MeasureTime infTime;
MeasureTime infendTime;
#endif

static request *inf_get_req_instance(const FSTYPE type, KEYT key, char *value, KEYT len,int mark, bool fromApp);

#ifdef interface_pq
pthread_mutex_t wq_lock;

static request *inf_get_multi_req_instance(const FSTYPE type, KEYT *key, char **value, int *len,int req_num,int mark, bool fromApp);

static bool qmanager_write_checking(processor * t,request *req){
	bool res=false;
	static int cnt=0;
	Redblack finding;
	pthread_mutex_lock(&t->qm_lock);
	if(rb_find_str(t->qmanager,req->key,&finding)){
		res=true;
		//copy value
	}
	else{
		rb_insert_str(t->qmanager,req->key,(void*)req);
	}
	pthread_mutex_unlock(&t->qm_lock);
#ifdef hash_dftl
	if(res) return res;
	pthread_mutex_lock(&rb_lock);
	rb_insert_str(rb_tree, req->key, NULL);
	pthread_mutex_unlock(&rb_lock);
#endif
	return res;
}

static bool qmanager_read_checking(processor *t,request *req){
	bool res=false;
	Redblack finding;
	static int read_q_hit=0;
	pthread_mutex_lock(&t->qm_lock);
	if(rb_find_str(t->qmanager,req->key,&finding)){
		res=true;
	}
	pthread_mutex_unlock(&t->qm_lock);
	return res;
}

static bool qmanager_delete(processor *t, request *req){
	bool res=false;
	Redblack finding;
	pthread_mutex_lock(&t->qm_lock);
	if(rb_find_str(t->qmanager,req->key,&finding)){
		res=true;
		rb_delete(finding);
	}
	else{
		abort();
	}
	pthread_mutex_unlock(&t->qm_lock);
	return res;
}

void *qmanager_find_by_algo(KEYT key){
	Redblack finding;
	for(int i=0; i<1; i++){
		processor *t=&mp.processors[i];
		if(rb_find_str(t->qmanager,key,&finding)){
			return finding->item;
		}
		continue;
	}
	return NULL;
}
#endif
void assign_req(request* req){
	bool flag=false;
	while(!flag){
		processor *t=&mp.processors[0];
#ifdef interface_pq
		switch(req->type){
			case FS_SET_T:
				if(qmanager_write_checking(t,req)){
					req->end_req(req);
					return;
				}
				else if(q_enqueue((void*)req,t->req_q)){
                    //printf("number of req : %d\n", t->req_q->size);
					flag=true;
				}
				break;
			case FS_GET_T:
				if(qmanager_read_checking(t,req)){
					if(!req->isstart) req->type_ftl=10;
					req->end_req(req);
					return;
				}
			default: //for read
				if(q_enqueue((void*)req,t->req_rq)){
                    //printf("number of req : %d\n", t->req_q->size);
					flag=true;
				}
				break;
		}
#else
		if(q_enqueue((void*)req,t->req_q)){
            //printf("number of req : %d\n", t->req_q->size);
			flag=true; continue;
		}
#endif
	}
	cl_release(inf_cond);
#ifdef BUSE_MEASURE
    if(req->type==FS_GET_T)
        MA(&infTime);
#endif
}
bool inf_assign_try(request *req){
	bool flag=false;
	for(int i=0; i<1; i++){
		processor *t=&mp.processors[i];
		while(q_enqueue((void*)req,t->retry_q)){
			cl_release(inf_cond);
			flag=true;
			break;
		}
	}
	return flag;
}

uint64_t inter_cnt;
bool force_write_start;
int write_stop;
static request *get_next_request(processor *pr){
	void *_inf_req=NULL;
	if(force_write_start || (write_stop && pr->req_q->size==QDEPTH) || sync_apps)
		write_stop=false;
	if((_inf_req=q_dequeue(pr->retry_q))) goto send_req; //check retry
#ifdef interface_pq
	else if((_inf_req=q_dequeue(pr->req_rq))) goto send_req; //check read 
	else if(pr->retry_q->size || write_stop) goto send_req; //check write stop
#endif
	else if((_inf_req=q_dequeue(pr->req_q))){
#ifdef interface_pq
		qmanager_delete(pr,(request*)_inf_req);
#endif
	}

send_req:
	return (request*)_inf_req;
}

void *p_main(void *__input){
	request *inf_req;
	processor *_this=NULL;
    int write_avoid=0;
	for(int i=0; i<1; i++){ //lsmtree-kv-variableSize
		if(pthread_self()==mp.processors[i].t_id){
			_this=&mp.processors[i];
		}
	}

	char thread_name[128]={0};
	sprintf(thread_name,"%s","inf_main_thread");
	pthread_setname_np(pthread_self(),thread_name);
	while(1){
		cl_grap(inf_cond);
        /*
		if(force_write_start ||(write_stop && _this->req_q->size==QDEPTH)){
			write_stop=false;
            write_avoid=0;
            force_write_start=0;
		}
        else{
            write_avoid++;
        }
        if(write_avoid==1024)
            force_write_start=1;

		if(mp.stopflag)
			break;
		if((inf_req=q_dequeue(_this->req_q))){

		}
#ifdef interface_pq
		else if(!(inf_req=q_dequeue(_this->req_rq))){
			pthread_mutex_lock(&wq_lock);
			if(_this->retry_q->size || write_stop || !(inf_req=q_dequeue(_this->req_q))){
				pthread_mutex_unlock(&wq_lock);
				//#else	//else if(!(inf_req=q_dequeue(_this->req_q))){
#endif	
				cl_release(inf_cond);
				continue;
			}
#ifdef interface_pq
			else{
				//req_flag=true;
			}
            */ //master
		if(mp.stopflag)
			break;
		if(!(inf_req=get_next_request(_this))){
			cl_release(inf_cond);
			continue;
		}
#ifdef QPRINT
        //printf("algo queue : %d\n", _this->req_q->size);
#endif

		inter_cnt++;
#ifdef CDF
		inf_req->isstart=true;
#endif
#ifdef LPRINT
        struct timeval algostart, algoend;
        gettimeofday(&algostart, NULL);
#endif
		static bool first_get=true;
		switch(inf_req->type){
			case FS_GET_T:
				if(first_get){
					first_get=false;
				}
				mp.algo->read(inf_req);
				break;
			case FS_SET_T:
				write_stop=mp.algo->write(inf_req);
				break;
			case FS_DELETE_T:
				mp.algo->remove(inf_req);
				break;
			case FS_RMW_T:
				mp.algo->read(inf_req);
				break;
			case FS_MSET_T:
				mp.algo->multi_set(inf_req,inf_req->num);
				break;
			case FS_MGET_T:
				mp.algo->multi_get(inf_req,inf_req->num);
				break;
			case FS_ITER_CRT_T:
				mp.algo->iter_create(inf_req);
				break;
			case FS_ITER_NXT_T:
				mp.algo->iter_next(inf_req);
				break;
			case FS_ITER_NXT_VALUE_T:
				mp.algo->iter_next_with_value(inf_req);
				break;
			case FS_ITER_ALL_T:
				mp.algo->iter_all_key(inf_req);
				break;
			case FS_ITER_ALL_VALUE_T:
				mp.algo->iter_all_value(inf_req);
				break;
			case FS_RANGEGET_T:
				mp.algo->range_query(inf_req);
				break;
			case FS_ITER_RLS_T:
				mp.algo->iter_release(inf_req);
				break;
			default:
				printf("wtf??, type %d\n", inf_req->type);
				inf_req->end_req(inf_req);
				break;
		}
#ifdef LPRINT
        gettimeofday(&algoend, NULL);
        printf("algo latency [sec, usec] : [%ld, %ld]\n", algoend.tv_sec-algostart.tv_sec, algoend.tv_usec-algostart.tv_usec);
#endif
	}
	return NULL;
}

bool inf_make_req_fromApp(char _type, KEYT _key, KEYT  offset, KEYT len,PTR _value,void *_req, void (*end_func)(uint32_t,uint32_t,void*)){
    request *req;
    value_set *value;
    static monitor *_m=NULL;
#ifdef BUSE_MEASURE
    if(_type==FS_GET_T)
        MS(&infTime);
#endif
	static bool start=false;
	if(!start){
		bench_init();
        _m=&_master->m[0];
        //_m->empty=false;
        //measure_init(&_m->benchTime);
        //MS(&_m->benchTime);
		bench_add(NOR,0,-1,-1);
		start=true;
	}
    //_m->m_num++;
	//value_set *value=(value_set*)malloc(sizeof(value_set));
    //value = req->value;
	if(_type!=FS_RMW_T){
        req=inf_get_req_instance(_type,_key,_value,len,0,true); //type, key, value, len, mark, fromApp
        value = req->value;
        ((struct buse*)_req)->value = value;
		//value->value=_value;
        if(_type!=FS_DELETE_T){
            value->rmw_value=NULL;
            value->offset=0;
        }
		//value->len=PAGESIZE;
	}else{
        req=inf_get_req_instance(_type,_key,_value,len,0,true);
        value = req->value;
		//value->value=(PTR)malloc(PAGESIZE);
		value->rmw_value=_value;
		value->offset=offset;
		//value->len=len;
	}
    if(_type!=FS_DELETE_T){
        value->length=len;
        value->dmatag=0;
        value->from_app=true;
    }

	//request *req=inf_get_req_instance(_type,_key,value,0,true);
	req->p_req=_req;
	req->p_end_req=end_func;

	cl_grap(flying);
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker); //make_fromapps
	measure_start(&req->latency_checker);//make_fromapps
#endif
	assign_req(req);
	return true;
}

bool inf_make_req_buse(char _type, KEYT _key, KEYT offset, KEYT len, PTR _value, void *_req, void (*end_func)(uint32_t, uint32_t, void*)){
  request *req;
  value_set *value;
  struct buse* buse_req = (struct buse*)_req;

  req = inf_get_req_instance(_type, _key, _value, len, 0, true);
  if(_type != FS_DELETE_T){
    value=req->value;
    value->offset=offset;
    buse_req->value = value;
  }
  req->p_req=_req;
  req->p_end_req=end_func;

  if(_type == FS_RMW_T){
    value->rmw_value = _value;
    value->rmw_len = buse_req->len;
  }

  cl_grap(flying);
  assign_req(req);
  return true;
}

void inf_init(int apps_flag, int total_num){
#ifdef BUSE_MEASURE
    measure_init(&infTime);
    measure_init(&infendTime);
#endif
	flying=cl_init(QDEPTH,false);
	inf_cond=cl_init(QDEPTH,true);
	mp.processors=(processor*)malloc(sizeof(processor)*1);
	for(int i=0; i<1; i++){
		processor *t=&mp.processors[i];
		pthread_mutex_init(&t->flag,NULL);
		pthread_mutex_lock(&t->flag);
		t->master=&mp;

		q_init(&t->retry_q,QSIZE);
#ifdef interface_pq
		q_init(&t->req_q,QSIZE);
		q_init(&t->req_rq,QSIZE);
		//q_init(&t->retry_q,QSIZE);
		pthread_mutex_init(&t->qm_lock,NULL);
		t->qmanager=rb_create();
#else
		q_init(&t->req_q,QSIZE);
#endif
		pthread_create(&t->t_id,NULL,&p_main,NULL);
	}


	pthread_mutex_init(&mp.flag,NULL);
	if(apps_flag){
		bench_init();
		bench_add(NOR,0,-1,total_num);
	}
	
	layer_info_mapping(&mp);
	mp.li->create(mp.li);
	mp.algo->create(mp.li,mp.algo);
	bb_checker_start(mp.li);
}

static request* inf_get_req_common(request *req, bool fromApp, int mark){
	static uint32_t seq_num=0;
	req->end_req=inf_end_req;
	req->isAsync=ASYNC;
	req->params=NULL;
	req->type_ftl = 0;
	req->type_lower = 0;
	req->before_type_lower=0;
	req->seq=seq_num++;
	req->special_func=NULL;
	req->added_end_req=NULL;
	req->p_req=NULL;
	req->p_end_req=NULL;
#ifndef USINGAPP
	req->mark=mark;
#endif

#ifdef hash_dftl
	req->hash_params = NULL;
#endif
	return req;
}

#ifdef KVSSD
static request *inf_get_req_instance(const FSTYPE type, KEYT key, char *_value, KEYT len,int mark,bool fromApp){
	request *req=(request*)malloc(sizeof(request));
	req->type=type;
//	req->key=key;
	req->ppa=0;
	req->multi_value=NULL;
	req->multi_key=NULL;
	req->num=len;
	req->cpl=0;
	
	req->key.len=key.len;
	req->key.key=(char*)malloc(key.len);
	memcpy(req->key.key,key.key,key.len);
	switch(type){
		case FS_DELETE_T:
			req->value=NULL;
			break;

		case FS_SET_T:
#ifdef DVALUE
			req->value=inf_get_valueset(NULL,FS_SET_T,len+key.len+sizeof(key.len));
			memcpy(&req->value->value[key.len+sizeof(key.len)],_value,len);
#else
			req->value=inf_get_valueset(NULL,FS_SET_T,PAGESIZE);
#endif
			memcpy(req->value->value,&key.len,sizeof(key.len));
			memcpy(&req->value->value[sizeof(key.len)],key.key,key.len);

			break;
		case FS_GET_T:

			req->value=inf_get_valueset(NULL,FS_GET_T,PAGESIZE);
			break;
		case FS_RANGEGET_T:
			req->multi_value=(value_set**)malloc(sizeof(value_set*)*req->num);
			for(int i=0; i<req->num; i++){
				req->multi_value[i]=inf_get_valueset(NULL,FS_GET_T,PAGESIZE);
			}
			break;
		case FS_MSET_T:
			break;
		case FS_MGET_T:
			break;
		default:
			break;
	}

	return inf_get_req_common(req,fromApp,mark);
}
#else
static request *inf_get_req_instance(const FSTYPE type, KEYT key, char *_value, KEYT len,int mark,bool fromApp){
	request *req=(request*)malloc(sizeof(request));
	req->type=type;
	req->key=key;	
	req->ppa=0;
  req->bulk_len = len;
	switch(type){
		case FS_DELETE_T:
			req->value=NULL;
			break;
		case FS_SET_T:
			req->value=inf_get_valueset(_value,FS_SET_T,len);
			break;
		case FS_GET_T:
			req->value=inf_get_valueset(_value,FS_GET_T,len);
			break;
    case FS_RMW_T:
			req->value=inf_get_valueset(NULL,FS_RMW_T,len);
		default:
			break;
	}
	return inf_get_req_common(req,fromApp,mark);
}
#endif


static request *inf_get_multi_req_instance(const FSTYPE type, KEYT *keys, char **_value, KEYT *len,int req_num,int mark, bool fromApp){
	request *req=(request*)malloc(sizeof(request));
	req->type=type;
	req->multi_key=keys;
	req->multi_value=(value_set**)malloc(sizeof(value_set*)*req_num);
	req->num=req_num;
	int i;
	switch(type){
		case FS_MSET_T:
			for(i=0; i<req_num; i++){
				req->value=inf_get_valueset(_value[i],FS_SET_T,len[i]);
			}
			break;
		case FS_RANGEGET_T:
			for(i=0; i<req_num; i++){
				req->value=inf_get_valueset(_value[i],FS_GET_T,len[i]);
			}
			break;
		default:
			break;
	}
	return inf_get_req_common(req,fromApp,mark);
}
#ifndef USINGAPP
bool inf_make_req(const FSTYPE type, const KEYT key, char *value, int len,int mark){
#else
bool inf_make_req(const FSTYPE type, const KEYT key,char* value){
#endif
#ifdef BUSE_MEASURE
    if(type==FS_GET_T){
        MS(&infTime);
    }
#endif
	request *req=inf_get_req_instance(type,key,value,len,mark,false);
	cl_grap(flying);
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker); //make_req
	measure_start(&req->latency_checker); //make_req
#endif
	assign_req(req);
	return true;
}


bool inf_make_multi_set(const FSTYPE type, KEYT *keys, char **values, int *lengths, int req_num, int mark){
	return 0;
}

bool inf_make_req_special(const FSTYPE type, const KEYT key, char* value, int len,uint32_t seq, void*(*special)(void*)){
	if(type==FS_RMW_T){
	}
	request *req=inf_get_req_instance(type,key,value,len,0,false);
	req->special_func=special;
	/*
	   static int cnt=0;
	   if(flying->now==1){
	   printf("[%d]will be sleep! type:%d\n",cnt++,type);
	   }*/
	cl_grap(flying);

	//set sequential
	req->seq=seq;
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker); //make_req_spe
	measure_start(&req->latency_checker); //make_req_spe
#endif

	assign_req(req);
	return true;
}

//int range_getcnt=0;
//static int end_req_num=0;
bool inf_end_req( request * const req){
#ifdef BUSE_MEASURE
    if(req->type==FS_GET_T)
        MS(&infendTime);
#endif
	if(req->type==FS_RMW_T){
		req->type=FS_SET_T;
		value_set *original=req->value;
		memcpy(&original->value[original->offset],original->rmw_value,original->rmw_len);
    //original code
		//value_set *temp=inf_get_valueset(req->value->value,FS_SET_T,req->value->length);
		//free(original->value);
		//req->value=temp;
		assign_req(req);
		return 1;
	}
#ifdef SNU_TEST
#else
	if(req->isstart){
		//bench_reap_data(req,mp.li);
	}else{
		bench_reap_nostart(req);
	}
#endif
#ifdef DEBUG
	printf("inf_end_req!\n");
#endif
	void *(*special)(void*);
	special=req->special_func;
	void **params;
	uint8_t *type;
	uint32_t *seq;
	if(special){
		params=(void**)malloc(sizeof(void*)*2);
		type=(uint8_t*)malloc(sizeof(uint8_t));
		seq=(uint32_t*)malloc(sizeof(uint32_t));
		*type=req->type;
		*seq=req->seq;
		params[0]=(void*)type;
		params[1]=(void*)seq;
		special((void*)params);
	}

	/*for range query*/
	if(req->added_end_req){
		req->added_end_req(req);
	}

	if(req->p_req){
		//req->p_end_req(req->p_req); master
		req->p_end_req(req->seq,req->ppa,req->p_req); //kv-variablesize
	}
	
	int i;//rw_check_type=0;

#if BULK==0
	switch(req->type){
		case FS_ITER_NXT_T:
			inf_free_valueset(req->value,FS_MALLOC_R);
			break;
		case FS_RANGEGET_T:
		case FS_ITER_NXT_VALUE_T:
	//		printf("end_req : %d\n",range_getcnt++);
#ifdef KVSSD
			free(req->key.key);
#endif
			for(i=0; i<req->num; i++){
				inf_free_valueset(req->multi_value[i],FS_MALLOC_R);
			}
			free(req->multi_value);
			break;

		case FS_GET_T:
#ifdef KVSSD
			free(req->key.key);
#endif
			if(req->value) inf_free_valueset(req->value,FS_MALLOC_R);
			break;
		case FS_NOTFOUND_T:
            break;
		case FS_SET_T:
			if(req->value) inf_free_valueset(req->value,FS_MALLOC_W);
			break;
	}
#endif

#ifdef BUSE_MEASURE
        if(req->type==FS_GET_T)
            MA(&infendTime);
#endif

	free(req);
	cl_release(flying);
	return true;
}

void inf_free(){
	bench_print();
	bench_free();
	mp.li->stop();
	mp.stopflag=true;
	int *temp;
	cl_free(flying);
	printf("result of ms:\n");
	printf("---\n");
	for(int i=0; i<1; i++){
		processor *t=&mp.processors[i];
		//		pthread_mutex_unlock(&inf_lock);
		while(pthread_tryjoin_np(t->t_id,(void**)&temp)){
			cl_release(inf_cond);
		}
		//pthread_detach(t->t_id);
		q_free(t->retry_q);
		q_free(t->req_q);
#ifdef interface_pq
		q_free(t->req_rq);
#endif

		pthread_mutex_destroy(&t->flag);
	}
	free(mp.processors);

#ifdef BUSE_MEASURE
    printf("infTime : ");
    measure_adding_print(&infTime);
    printf("infendTime : ");
    measure_adding_print(&infendTime);
#endif
	mp.algo->destroy(mp.li,mp.algo);
	mp.li->destroy(mp.li);
}

void inf_print_debug(){

}

bool inf_make_multi_req(char type, KEYT key,KEYT *keys,uint32_t iter_id,char **values,uint32_t lengths,bool (*added_end)(struct request *const)){
	request *req=inf_get_req_instance(type,key,NULL,PAGESIZE,0,false);
	cl_grap(flying);
	uint32_t i;
	switch(type){
		case FS_MSET_T:
			/*should implement*/
			break;
		case FS_ITER_CRT_T:
			break;
		case FS_ITER_NXT_T:
			req->value=inf_get_valueset(NULL,FS_MALLOC_R,PAGESIZE);
			req->num=lengths;
			break;
		case FS_ITER_NXT_VALUE_T:
			req->multi_value=(value_set**)malloc(sizeof(value_set*)*lengths);
			for(i=0; i < lengths; i++){
				req->multi_value[i]=inf_get_valueset(NULL,FS_GET_T,PAGESIZE);
			}
			req->num=lengths;
			break;
		case FS_ITER_RLS_T:

			break;
		default:
			printf("error in inf_make_multi_req\n");
			return false;
	}
	req->ppa=iter_id;
	req->added_end_req=added_end;
	req->isstart=false;
	measure_init(&req->latency_checker); //make_multi_req
	measure_start(&req->latency_checker); //make_multi_req
	assign_req(req);
	return true;
}
bool inf_iter_create(KEYT start,bool (*added_end)(struct request *const)){
	return inf_make_multi_req(FS_ITER_CRT_T,start,NULL,0,NULL,PAGESIZE,added_end);
}
bool inf_iter_next(uint32_t iter_id,char **values,bool (*added_end)(struct request *const),bool withvalue){
#ifdef KVSSD
	static KEYT null_key={0,};
#else
	static KEYT null_key=0;
#endif
	if(withvalue){
		return inf_make_multi_req(FS_ITER_NXT_VALUE_T,null_key,NULL,iter_id,values,0,added_end);
	}
	else{
		return inf_make_multi_req(FS_ITER_NXT_T,null_key,NULL,iter_id,values,0,added_end);
	}
}
bool inf_iter_release(uint32_t iter_id, bool (*added_end)(struct request *const)){
#ifdef KVSSD
	static KEYT null_key={0,};
#else
	static KEYT null_key=0;
#endif
	return inf_make_multi_req(FS_ITER_RLS_T,null_key,NULL,iter_id,NULL,0,added_end);
}

#ifdef KVSSD
bool inf_make_req_apps(char type, char *keys, uint8_t key_len,char *value,int len, int seq,void *_req,void (*end_req)(uint32_t,uint32_t,void*)){
	KEYT t_key;
	t_key.key=keys;
	t_key.len=key_len;
	request *req=inf_get_req_instance(type,t_key,value,len,0,false);
	req->seq=seq;
	req->p_req=_req;
	req->p_end_req=end_req;
	
	cl_grap(flying);
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker);//make_req_apps
	measure_start(&req->latency_checker);//make_req_apps
#endif
	assign_req(req);
	return true;
}
bool inf_make_range_query_apps(char type, char *keys, uint8_t key_len,int seq, int length,void *_req,void (*end_req)(uint32_t,uint32_t, void*)){
	KEYT t_key;
	t_key.key=keys;
	t_key.len=key_len;
	request *req=inf_get_req_instance(type,t_key,NULL,length,0,false);
	req->seq=seq;
	req->p_req=_req;
	req->p_end_req=end_req;
	
	cl_grap(flying);
//	printf("seq:%d\n",req->seq);
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker);//make_range
	measure_start(&req->latency_checker);//make_range
#endif
	assign_req(req);
	return true;
}

bool inf_make_mreq_apps(char type, char **keys, uint8_t *key_len, char **values,int num, int seq, void *_req,void (*end_req)(uint32_t,uint32_t, void*)){
#ifdef KVSSD
	static KEYT null_key={0,};
#else
	static KEYT null_key=0;
#endif
	request *req=inf_get_req_instance(type,null_key,NULL,PAGESIZE,0,false);
	req->multi_key=(KEYT*)malloc(sizeof(KEYT)*num);
	req->multi_value=(value_set**)malloc(sizeof(value_set*)*num);
	for(int i=0; i<num; i++){
		req->multi_key[i].key=keys[i];
		req->multi_key[i].len=key_len[i];
		req->multi_value[i]=inf_get_valueset(values[i],FS_MALLOC_R,PAGESIZE);
	}
	req->num=num;
	req->seq=seq;
	req->p_req=_req;
	req->p_end_req=end_req;
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker);//make_mreq
	measure_start(&req->latency_checker);//make_mreq
#endif
	assign_req(req);
	return true;
}

bool inf_iter_req_apps(char type, char *prefix, uint8_t key_len,char **value, int seq,void *_req, void (*end_req)(uint32_t,uint32_t, void *)){
#ifdef KVSSD
	static KEYT null_key={0,};
#else
	static KEYT null_key=0;
#endif
	request *req=inf_get_req_instance(type,null_key,NULL,0,0,false);
	switch(type){
		case FS_ITER_ALL_VALUE_T:
		case FS_ITER_ALL_T:
			req->key.key=prefix;
			req->key.len=key_len;
			req->app_result=value;
			break;
		case FS_ITER_RLS_T:
			break;
		case FS_ITER_CRT_T:
		case FS_ITER_NXT_T:
		case FS_ITER_NXT_VALUE_T:
			printf("not implemented");
			abort();
			break;
	}
	req->seq=seq;
	req->p_req=_req;
	req->p_end_req=end_req;
#ifdef CDF
	req->isstart=false;
	measure_init(&req->latency_checker); //make_iter
	measure_start(&req->latency_checker);//make_iter
#endif
	assign_req(req);
	return true;
}
#endif

value_set *inf_get_valueset(PTR in_v, int type, uint32_t length){
	value_set *res=(value_set*)malloc(sizeof(value_set));
	//check dma alloc type
#if BULK
  res->dmatag=0;
  res->length=length;
  res->rmw_value=NULL;
  res->rmw_len=0;

  if(type==FS_SET_T)
    res->value=in_v;
  else
    F_malloc((void**)&res->value, length, type);

  return res;
#else

	if(length==PAGESIZE)
		res->dmatag=F_malloc((void**)&(res->value),PAGESIZE,type);
	else{
		length=(length/PIECE+(length%PIECE?1:0))*PIECE;
		res->dmatag=-1;
		res->value=(PTR)malloc(length);
	}
	res->length=length;

	res->from_app=false;
	if(in_v){
		memcpy(res->value,in_v,length);
	}
	else{
		memset(res->value,0,length);
	}
	return res;
#endif
}

void inf_free_valueset(value_set *in, int type){
#ifdef BULK
  if(in->value)
    F_free((void*)in->value, in->dmatag, type);
  
  if(in->rmw_value)
    F_free((void*)in->rmw_value, in->dmatag, type);

  free(in);

  return;
#endif

	if(!in->from_app){
		if(in->dmatag==-1){
			free(in->value);
		}
		else{
			F_free((void*)in->value,in->dmatag,type);
		}
	}
	free(in);
}
