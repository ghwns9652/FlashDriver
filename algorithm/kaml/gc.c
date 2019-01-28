#include <stdlib.h>
#include <stdio.h>
#include "gc.h"

extern int OOB[_NOP];
extern int mapping_table[_NOP];	// use LPA as index, use 1 block as log block
extern uint8_t garbage_table[_NOP/8];

extern int log_seg_num; 
extern int gc_read_cnt;
extern int gc_target_cnt;

int garbage_collection(int reserv_ppa_start, int erase_seg_num)
{
	//erase_seg_num = delete_heap(&heap);
	int invalid_cnt = 0;
	
	gc_read_cnt=0;
	gc_target_cnt=0;
	
	int start_page_num = erase_seg_num * _PPS;
	int end_page_num = (erase_seg_num+1) * _PPS;
	uint8_t bit_compare;
	
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = pftl_end_req;
	//my_req->type = DATAW;
	my_req->params = (void*)params;


	/*            valid check & copy                */
	value_set *value=inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	for(int i=start_page_num; i<end_page_num; i++){ 	//valid checking
		if (garbage_table[i/8] & (1<<i%8)) {  // 1: invalid
			invalid_cnt++ 
			if(invalid_cnt == _PPS){	// all page is invalid
				algo_pftl.li->trim(start_page_num, ASYNC);  //delete all segment
				return start_page_num; 
			}
		}
		else {//copy on reserved segment	// 0: valid
			mapping_table[OOB[i]] = reserv_ppa_start;	// mapping_table update
			OOB[reserv_ppa_start] = OOB[i];			// OOB update

			//GC_R
			my_req->type = GC_R;
			gc_read_cnt++;
			algo_pftl.li->read(i, PAGESIZE, value, 1, my_req);		
			
			//waiting for gc_read
			gc_general_waiting();
			
			//GC_W
			my_req->type = GC_W;
			algo_pftl.li->write(reserv_ppa_start, PAGESIZE, value, 1, my_req);					
			
			//increase reserved ppa number
			reserv_ppa_start++;
		}

	}
	
	/*      delete old segment	*/
	algo_pftl.li->trim(start_page_num, ASYNC);
	
	// return update reserv segment number
	log_seg_num = start_page_num / _PPS;

	return reserv_ppa_start;	
}

void gc_general_waiting()
{
	//spin lcok for gc_read
	while(gc_read_cnt != gc_target_cnt) { }

	gc_read_cnt = 0;
	gc_target_cnt = 0;
}
