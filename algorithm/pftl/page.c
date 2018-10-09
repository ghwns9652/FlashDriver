#include "page.h"
#include "../../bench/bench.h"

struct algorithm algo_pbase={
	.create = pbase_create,
	.destroy = pbase_destroy,
	.get = pbase_get,
	.set = pbase_set,
	.remove = pbase_remove
};

//heap globals.
b_queue *free_b;
Heap *b_heap;

TABLE *page_TABLE; //mapping table.
P_OOB *page_OOB;   //OOB area.

//blockmanager globals.
BM_T *BM;
Block *reserved;    //reserved.

//buffering & caching.
w_buff *page_wbuff;
r_cache *page_rcache;

//queueing.
pthread_t pbase_main_thread;
sem_t empty;
sem_t full;
algo_queue* page_queue;
extern void* pbase_main(void*);

int in = -1;
int out = -1;
int32_t end_flag;

//global for macro.
int32_t _g_nop;
int32_t _g_nob;
int32_t _g_ppb;

int32_t gc_load;
int32_t gc_count;

int32_t buff_count;

uint32_t pbase_create(lower_info* li, algorithm *algo){
	/*
	   initializes table, oob and blockmanager.
	   alloc globals for macro.
	   init heap.
	*/
	_g_nop = _NOP;
	_g_nob = _NOS;
	_g_ppb = _PPS;
	gc_count = 0;
	buff_count = 0;
	end_flag = 0;
	page_TABLE = (TABLE*)malloc(sizeof(TABLE) * _g_nop);
	page_OOB = (P_OOB*)malloc(sizeof(P_OOB) * _g_nop);
	page_queue = (algo_queue*)malloc(sizeof(algo_queue)*ALGO_QUEUESIZE);

	sem_init(&empty,0,4);
	sem_init(&full,0,0);
	pthread_create(&pbase_main_thread,NULL,pbase_main,NULL);
	
	algo->li = li;

	for(int i=0;i<_g_nop;i++){
     	page_TABLE[i].ppa = -1;
		page_OOB[i].lpa = -1;
	}//init table, oob and vbm.

	BM = BM_Init(1, 1);

	reserved = &BM->barray[0];
	BM_Queue_Init(&free_b);
	for(int i=1;i<_g_nob;i++){
		BM_Enqueue(free_b, &BM->barray[i]);
	}
	b_heap = BM_Heap_Init(_g_nob - 1);//total size == NOB - 1.
	
	BM->harray[0] = b_heap;
	BM->qarray[0] = free_b;
	return 0;
}

void pbase_destroy(lower_info* li, algorithm *algo){
	/*
	 * frees allocated mem.
	 * destroys blockmanager.
	 */
	printf("gc count: %d\n", gc_count);
	end_flag = 1;
//	pthread_join(pbase_main_thread,NULL);
//	pthread_exit(NULL);
	sem_destroy(&empty);
	sem_destroy(&full);
	BM_Free(BM);
	free(page_OOB);
	free(page_TABLE);
	free(page_queue);
}

void *pbase_end_req(algo_req* input){
	/*
	 * end req differs according to type.
	 * frees params and input.
	 * in some case, frees inf_req.
	 */
	pbase_params *params = (pbase_params*)input->params;
	value_set *temp_set = params->value;
	request *res = input->parents;

	switch(params->type){
		case DATA_R:
			if(res){
				res->end_req(res);
			}
			break;
		case DATA_W:
			if(res){
				res->end_req(res);
			}
			break;
		case GC_R:
			gc_load++;	
			break;
		case GC_W:
			inf_free_valueset(temp_set, FS_MALLOC_W);
			break;
	}
	free(params);
	free(input);
	return NULL;
}
uint32_t pbase_get(request* const req){
	printf("get called.\n");
	sem_wait(&empty);
	in++;
	in %= 4;
	page_queue[in].req = req;
	page_queue[in].rw = 0;
	sem_post(&full);
	return 0;
}

uint32_t pbase_set(request* const req){
//	sleep(1);
	printf("set called.\n");
	sem_wait(&empty);
	in++;
	in %= 4;
	page_queue[in].req = req;
	page_queue[in].rw = 1;
	sem_post(&full);
	return 0;
}


uint32_t pbase_get_fromqueue(request* const req){
	/*
	 * gives pull request to lower level.
	 * reads mapping data.
	 * !!if not mapped, does not pull!!
	 */
	int32_t lpa = 0;
	int32_t ppa = 0;
	int32_t buffer_idx = 0;
	int8_t flag = 0;
	/*buffer check code.
	for(int i=0;i<ALGO_BUFSIZE;i++){
		if(req->key == page_wbuff[i].lpa){
			buffer_idx = i;
			flag = 1;
		}
	}
	*/

	/*buffer / cache check.
	if(0){//if target lpa is in write buffer,
		bench_algo_start(req);
		memcpy(req->value->value,page_wbuff[buffer_idx].req->value->value,sizeof(PAGESIZE));
		printf("return from buffer : %c\n",req->value->value[0]);
		bench_algo_end(req);
		req->end_req(req);
		flag = 0;
		return 0;
	}
	else if(0){//if target lpa is in read cache,

		req->end_req(req);
		return 0;
	}*/

	bench_algo_start(req);
	lpa = req->key;
	ppa = page_TABLE[lpa].ppa;
	if(ppa == -1){
		bench_algo_end(req);
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}
	bench_algo_end(req);	
	algo_pbase.li->pull_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));
	/*read caching tactic : LRU*/
	/*read caching*/
	/*!read cacheing*/
	
	return 0;
}

uint32_t pbase_set_fromqueue(request* const req){
	/*
	 * gives push request to lower level.
	 * write mapping data, AFTER push request.
	 * need to write OR update table, oob, VBM.
	 * if necessary, allocation may perf garbage collection.
	 */

	/*write buffering.
	if(buff_count != ALGO_BUFSIZE){
		page_wbuff[buff_count].req = req;
		page_wbuff[buff_count].lpa = req->key;
		buff_count++;
	}
	!write buffering.*/

	//if(buff_count != 4){return 0;}//if buffer is not full, exit.
	//else{//if buffer is full, flush.
	int32_t lpa;
	int32_t ppa;
	bench_algo_start(req);
	lpa = req->key;
	ppa = alloc_page();//may perform garbage collection.
	bench_algo_end(req);
	algo_pbase.li->push_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req));
	if(page_TABLE[lpa].ppa != -1){//already mapped case.(update)
		BM_InvalidatePage(BM,page_TABLE[lpa].ppa);
	}
	page_TABLE[lpa].ppa = ppa;
	BM_ValidatePage(BM,ppa);
	page_OOB[ppa].lpa = lpa;
	
	return 0;
}

uint32_t pbase_remove(request* const req){
	/*reset info. not being used now. */

	int32_t lpa;

	bench_algo_start(req);
	lpa = req->key;
	page_TABLE[lpa].ppa = -1; //reset to default.
	page_OOB[lpa].lpa = -1; //reset reverse_table to default.
	bench_algo_end(req);
	return 0;
}
