#include "factory.h"
#include "../../include/lsm_settings.h"
#include "footer.h"
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<limits.h>

extern lsmtree LSM;
ftryM factory;
bool factory_init(){
	factory.processors=(ftryP*)malloc(sizeof(ftryP)*FTHREAD);
	memset(factory.processors,0,sizeof(ftryP)*FTHREAD);
	for(int i=0; i<FTHREAD; i++){
		factory.processors[i].master=&factory;
		pthread_mutex_init(&factory.processors[i].flag,NULL);
		pthread_mutex_lock(&factory.processors[i].flag);
		q_init(&factory.processors->q,FQSIZE);
		pthread_create(&factory.processors[i].t_id,NULL,factory_main,NULL);
	}
	factory.stopflag=false;
	return true;
}

void factory_free(){
	factory.stopflag=true;
	int *temp;
	for(int i=0; i<FTHREAD; i++){
		ftryP *t=&factory.processors[i];
		pthread_join(t->t_id,(void**)&temp);
		q_free(t->q);
	}
	free(factory.processors);
}

void ftry_assign(algo_req *req){
	bool flag=false;
	while(1){
#ifdef LEAKCHECK
		sleep(1);
#endif
		for(int i=0; i<FTHREAD; i++){
			ftryP* proc=&factory.processors[i];
			if(q_enqueue((void*)req,proc->q)){
				flag=true;
				break;
			}
		}
		if(flag) break;
	}
}

void *factory_main(void *input){
	void *_req;
	algo_req *lsm_req=NULL;
	lsm_params *l_params=NULL;
	ftryP *_this=NULL;
	for(int i=0; i<FTHREAD; i++){
		if(pthread_self()==factory.processors[i].t_id)
			_this=&factory.processors[i];
	}
	while(1){
#ifdef LEAKCHECK
		sleep(1);
#endif
		if(factory.stopflag)
			break;
		if(!(_req=q_dequeue(_this->q))){
			continue;
		}
		lsm_req=(algo_req*)_req;
		l_params=(lsm_params*)lsm_req->params;
		request* req=lsm_req->parents;
		value_set *origin_valueset=req->value;
		switch(l_params->lsm_type){
			case SDATAR:
				origin_valueset=req->value;
				req->value=f_grep_data(req->key,origin_valueset->ppa,origin_valueset->value);
				if(req->value==NULL){
					req->type=FS_NOTFOUND_T;
					req->value=origin_valueset;
				}
				else{
					inf_free_valueset(origin_valueset,FS_MALLOC_R);
				}
				break;
			case RANGER:
				break;
			default:
				printf("missing request in factory\n");
				break;
		}
		if(req){
			req->end_req(req);
		}
		free(l_params);
		free(lsm_req);
	}
	return NULL;
}