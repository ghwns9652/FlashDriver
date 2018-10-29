#include "page.h"

void* pbase_main(void* arg){
	printf("thread created!\n");
	/* a. until the queue becomes empty,
	   b. until the pbase_destroy is called,
	   get request from algo_queue.
	   process request. 
	 */
	while(1){
		if(end_flag == 1)
			break;

		if(!sem_trywait(&full)){
			out++;
			out %= 4;
			if(page_queue[out].rw == 0){
				pbase_get_fromqueue(page_queue[out].req);
			}
			else if(page_queue[out].rw == 1){
			pbase_set_fromqueue(page_queue[out].req);
			}
			else
				printf("not read nor write\n");
			sem_post(&empty);
			printf("consumed. count :%d\n",out);
		}
	}
}
