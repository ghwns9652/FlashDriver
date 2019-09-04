#include "lsmtree.h"
#include "compaction.h"
#include "skiplist.h"
#include "page.h"
#include "bloomfilter.h"
#include "nocpy.h"
#include "lsmtree_scheduling.h"
#include "../../include/utils/thpool.h"
extern lsmtree LSM;
pl_run *make_pl_run_array(level *t, uint32_t *num){
	//first lock
	*num=LSM.lop->get_number_runs(t);
	pl_run *res=(pl_run*)malloc(sizeof(pl_run)*(*num));
	/*if(t->idx<LSM.LEVELCACHING){
		run_t **tr;
		for(int i=0; tr[i]!=NULL; i++){
			res[i].r=tr[i];
			res[i].lock=(fdriver_lock_t*)malloc(sizeof(fdriver_lock_t));
			fdriver_mutex_init(res[i].lock);
		}
	}else{*/
		lev_iter* iter=LSM.lop->get_iter(t,t->start,t->end);
		run_t *now;
		int i=0;
		while((now=LSM.lop->iter_nxt(iter))){
			res[i].r=now;
			res[i].lock=(fdriver_lock_t*)malloc(sizeof(fdriver_lock_t));
			fdriver_mutex_init(res[i].lock);
			i++;
		}
//	}
	return res;
}

void pl_run_free(pl_run *pr, uint32_t num ){
	for(int i=0; i<num; i++){
		fdriver_destroy(pr[i].lock);
		free(pr[i].lock);
		htable_read_postproc(pr[i].r);
	}
	free(pr);
}

void *level_insert_write(level *t, run_t *data){
	compaction_htable_write_insert(t,data,false);
	free(data);
	return NULL;
}
static threadpool bg_read_th; 
void bg_start(void *arg, int id){
	void **p=(void**)arg;
	run_t **r=(run_t**)p[0];
	fdriver_lock_t **locks=(fdriver_lock_t**)p[1];
	
	algo_req *areq;
	lsm_params *params;
	for(int i=0; r[i]!=NULL; i++){
		areq=(algo_req*)malloc(sizeof(algo_req));
		params=(lsm_params*)malloc(sizeof(lsm_params));

		params->lsm_type=BGREAD;
		params->value=inf_get_valueset(NULL,FS_MALLOC_R,PAGESIZE);
		params->target=(PTR*)&r[i]->cpt_data;
		params->ppa=r[i]->pbn;
		params->lock=locks[i];

		areq->parents=NULL;
		areq->end_req=lsm_end_req;
		areq->params=(void*)params;
		areq->type_lower=0;
		areq->rapid=false;
		areq->type=HEADERR;
		if(LSM.nocpy) r[i]->cpt_data->nocpy_table=nocpy_pick(r[i]->pbn);
		LSM.li->read(r[i]->pbn,PAGESIZE,params->value,ASYNC,areq);
	}
	free(r);
	free(locks);
	return;
}

uint32_t pipe_partial_leveling(level *t, level *origin, leveling_node* lnode, level *upper){
	compaction_sub_pre();

	static bool thpool_init_flag=false;
	if(!thpool_init_flag){
		thpool_init_flag=true;
		bg_read_th=thpool_init(1);
	}

	uint32_t u_num=0, l_num=0;
	pl_run *u_data=NULL, *l_data=NULL;
	if(upper){
		u_data=make_pl_run_array(upper,&u_num);
	}
	l_data=make_pl_run_array(origin,&l_num);

	uint32_t all_num=u_num+l_num+1;
	run_t **read_target=(run_t **)malloc(sizeof(run_t*)*all_num);
	fdriver_lock_t **lock_target=(fdriver_lock_t**)malloc(sizeof(fdriver_lock_t*)*all_num);

	int cnt=0;
	uint32_t min_num=u_num<l_num?u_num:l_num;
	for(int i=0; i<min_num; i++){
		run_t *t; fdriver_lock_t *tl;
		for(int j=0; j<2; j++){
			t=!j?u_data[i].r:l_data[i].r;
			tl=!j?u_data[i].lock:l_data[i].lock;
			if((!j && upper && upper->idx<LSM.LEVELCACHING) || htable_read_preproc(t)){
				continue;
			}
			else{
				fdriver_lock(tl);
				read_target[cnt]=t;
				lock_target[cnt]=tl;
				cnt++;
			}
		}
	}
	
	for(int i=min_num; i<u_num; i++){
		if((upper && upper->idx<LSM.LEVELCACHING) || htable_read_preproc(u_data[i].r)){
			continue;
		}
		else{
			fdriver_lock(u_data[i].lock);
			read_target[cnt]=u_data[i].r;
			lock_target[cnt]=u_data[i].lock;
			cnt++;
		}
	}

	for(int i=min_num; i<l_num; i++){
		if(htable_read_preproc(l_data[i].r)){
			continue;
		}
		else{
			fdriver_lock(l_data[i].lock);
			read_target[cnt]=l_data[i].r;
			lock_target[cnt]=l_data[i].lock;
			cnt++;
		}
	}
	
	read_target[cnt]=NULL;
	void *arg[2];
	arg[0]=(void*)read_target;
	arg[1]=(void*)lock_target;
	thpool_add_work(bg_read_th,bg_start,(void*)arg);
	//compaction_bg_htable_bulkread(read_target,lock_target);

	LSM.lop->partial_merger_cutter(lnode?lnode->mem:NULL,u_data,l_data,u_num,l_num,t,level_insert_write);

	compaction_sub_post();

	while(thpool_num_threads_working(bg_read_th)==1);

	if(u_data) pl_run_free(u_data,u_num);
	pl_run_free(l_data,l_num);
	return 1;
}	
