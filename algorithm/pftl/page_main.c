#include "page.h"
#include "../../bench/bench.h"

void* pbase_main(void* arg){
	printf("thread created!\n");
	/* a. until the queue becomes empty,
	   b. until the pbase_destroy is called,
	   get request from algo_queue.
	   process request.

	   check if anything stays in buffer.
	   if something is in buffer, flush it.

	 */
	int flush_end_check = 0;

	while(1){
		if(end_flag == 1)
			break;

		if(!sem_trywait(&full)){
			out++;
			out %= ALGO_QUEUESIZE;
			if(page_queue[out].rw == 0){
				pbase_get_fromqueue(page_queue[out].req);
			}
			else if(page_queue[out].rw == 1){
			pbase_set_fromqueue(page_queue[out].req);
			}
			else
				printf("not read nor write\n");
			sem_post(&empty);
	//		printf("consumed. count :%d\n",out);
			flush_end_check = 1;
		}
		if(flush_end_check == 1)
			gettimeofday(&flush_flag_end,NULL);

		if(flush_flag_end.tv_sec - flush_flag_start.tv_sec > 5){
			printf("time to flush!\n");
			//printf("%d, %d\n",flush_flag_end.tv_sec,flush_flag_start.tv_sec);
			for(int i=0;i<ALGO_BUFSIZE;i++){
				if(page_wbuff[i].lpa == -1){break;}
				int32_t lpa;
				int32_t ppa;
				request* const iter_req = page_wbuff[i].req;
				bench_algo_start(iter_req);
				lpa = iter_req->key;
				ppa = alloc_page();//may perform garbage collection.
				bench_algo_end(iter_req);
				algo_pbase.li->push_data(ppa, PAGESIZE, iter_req->value, ASYNC, assign_pseudo_req(DATA_W, NULL, iter_req));
				if(page_TABLE[lpa].ppa != -1){//already mapped case.(update)
					BM_InvalidatePage(BM,page_TABLE[lpa].ppa);
				}
				page_TABLE[lpa].ppa = ppa;
				BM_ValidatePage(BM,ppa);
				page_OOB[ppa].lpa = lpa;

				//reset buffer.
				page_wbuff[i].lpa = -1;
				page_wbuff[i].req = NULL;
			}
		}
	}
}
