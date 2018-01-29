#include "../../interface/interface.h"
#include <pthread.h>

master_processor mp;
static void algo_assign_req(request* req){
	bool flag=false;
	if(!req->isAsync){
		pthread_mutex_init(&req->async_mutex,NULL);
		pthread_mutex_lock(&req->async_mutex);
	}

	while(!flag){
		for(int i=0; i<THREADSIZE; i++){
			processor *t=&mp.processors[i];
			if(q_enqueue(req,t->req_q)){
				flag=true;
				break;
			}
			else{
				flag=false;
				continue;
			}
		}
		//sleep or nothing
#ifdef LEAKCHECK
		sleep(1);
#endif
	}

	if(!req->isAsync){
		pthread_mutex_lock(&req->async_mutex);
		pthread_mutex_destroy(&req->async_mutex);
	}
}


