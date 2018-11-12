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
struct timeval flush_flag_start;
struct timeval flush_flag_end;
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

	page_TABLE = (TABLE*)malloc(sizeof(TABLE) * _g_nop * ALGO_SEGNUM);
	page_OOB = (P_OOB*)malloc(sizeof(P_OOB) * _g_nop * ALGO_SEGNUM);

	page_queue = (algo_queue*)malloc(sizeof(algo_queue)*ALGO_QUEUESIZE);
	
	page_wbuff = (w_buff*)malloc(sizeof(w_buff)*ALGO_BUFSIZE);

	sem_init(&empty,0,ALGO_QUEUESIZE);
	sem_init(&full,0,0);
	pthread_create(&pbase_main_thread,NULL,pbase_main,NULL);
	
	algo->li = li;

	for(int i=0;i<_g_nop*ALGO_SEGNUM;i++){
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
	pthread_join(pbase_main_thread,NULL);
//	pthread_exit(NULL);
	sem_destroy(&empty);
	sem_destroy(&full);
	BM_Free(BM);
	free(page_OOB);
	free(page_TABLE);
	free(page_queue);
	free(page_wbuff);
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
//	printf("get called.\n");
	sem_wait(&empty);
	in++;
	in %= ALGO_QUEUESIZE;
	page_queue[in].req = req;
	page_queue[in].rw = 0;
	sem_post(&full);
	return 0;
}

uint32_t pbase_set(request* const req){
//	sleep(1);
//	printf("set called.\n");
	sem_wait(&empty);
	in++;
	in %= ALGO_QUEUESIZE;
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

	int8_t lppa = 0; //large-page compatibility::determines real physical.
	int8_t offset = 0;

	int8_t flag = 0; //buffer check flag.

	//buffer check code.
	for(int i=0;i<ALGO_BUFSIZE;i++){//linear search.
		if(req->key == page_wbuff[i].lpa){
			buffer_idx = i;
			flag = 1;
		}
	}
	//!buffer check code.

	if(flag == 1){//buffer read case.
		bench_algo_start(req); 
		memcpy(req->value->value,page_wbuff[buffer_idx].req->value->value,sizeof(PAGESIZE));
		printf("return from buffer : %c\n",req->value->value[0]);
		bench_algo_end(req);
		req->end_req(req);
		flag = 0;
		return 0;
	}

	bench_algo_start(req);
	lpa = req->key;
	ppa = page_TABLE[lpa].ppa;
	
	if(LARGE_PAGE){
		lppa = ppa / ALGO_SEGNUM;
		offset = ppa % ALGO_SEGNUM;
	}
	else{
		lppa = ppa;
		offset = 0;
	}

	if(ppa == -1){
		bench_algo_end(req);
//		printf("not mapped..!\n");
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}
	bench_algo_end(req);	
	algo_pbase.li->pull_data(lppa, PAGESIZE/ALGO_SEGNUM, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));

	
	return 0;
}

uint32_t pbase_set_fromqueue(request* const req){
	/*
	 * gives push request to lower level.
	 * write mapping data, AFTER push request.
	 * need to write OR update table, oob, VBM.
	 * if necessary, allocation may perf garbage collection.
	 */

	//write buffering.
	if(buff_count != ALGO_BUFSIZE){
		page_wbuff[buff_count].req = req;
		page_wbuff[buff_count].lpa = req->key;
		buff_count++;
		gettimeofday(&flush_flag_start,NULL);
	}
	//!write buffering.

	if(buff_count != ALGO_BUFSIZE){return 0;}//if buffer is not full, exit.

	else{//if buffer is full, flush.

		for(int i=0;i<ALGO_BUFSIZE;i++){
			int32_t lpa;
			int32_t ppa;
			int32_t lppa; //large-page compatibility.
			int8_t offset;

			int8_t _offset;
			int8_t _page;

			request* const iter_req = page_wbuff[i].req;

			bench_algo_start(iter_req);
			lpa = iter_req->key;
			ppa = alloc_page();//may perform garbage collection.
			bench_algo_end(iter_req);
			
			if(LARGE_PAGE){//large_page logic.
				_offset = i % ALGO_SEGNUM;
				_page = i / ALGO_SEGNUM;

				if(_offset != 0){//merge datum on 0-offset request.
					lppa = ppa / ALGO_SEGNUM;
					offset = ppa % ALGO_SEGNUM;
					memcpy(&(page_wbuff[i-_offset].req->value->value[_offset*ALGO_SEGSIZE])							 ,page_wbuff[i].req->value->value,ALGO_SEGSIZE);
					iter_req->end_req(iter_req);
					if(_offset == ALGO_SEGNUM - 1){//last component will push data into dev.
						algo_pbase.li->push_data(lppa,PAGESIZE,
								page_wbuff[i-_offset].req->value,ASYNC,
								assign_pseudo_req(DATA_W,NULL,page_wbuff[i-_offset].req));
					}
				}
			}
			else{
				algo_pbase.li->push_data(lppa, PAGESIZE, iter_req->value, ASYNC, 
										 assign_pseudo_req(DATA_W, NULL, iter_req));
			}

			if(page_TABLE[lpa].ppa != -1){//already mapped case.(update)
				BM_InvalidatePage(BM,page_TABLE[lpa].ppa);
			}
			page_TABLE[lpa].ppa = ppa;
			BM_ValidatePage(BM,ppa);
			page_OOB[ppa].lpa = lpa;

			//reset buffer.
		}
		for(int j=0;j<ALGO_BUFSIZE;j++){
			page_wbuff[j].lpa = -1;
			page_wbuff[j].req = NULL;
		}

		buff_count = 0;
		
	}	
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
