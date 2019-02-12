#include <stdlib.h>
#include <stdio.h>
#include "gc.h"

extern struct algorithm __normal;
extern int OOB[_NOP];
extern int mapping_table[_NOP];	// use LPA as index, use 1 block as log block
extern uint8_t garbage_table[_NOP/8];

extern int log_seg_num; 
extern int gc_read_cnt;
extern int gc_target_cnt;

int garbage_collection(int reserv_ppa_start, int erase_seg_num)
{
	int invalid_cnt = 0;
	gc_read_cnt=0;
	gc_target_cnt=0;
	
	int start_page_num = erase_seg_num * _PPS;	// reserved segment
	int end_page_num = (erase_seg_num+1) * _PPS;
	uint8_t bit_compare;
	
	for(uint32_t i = start_page_num; i < end_page_num; i++){ 	//valid checking
		value_set *value_w;
		bit_compare = garbage_table[i/8];

		if (bit_compare & (1 << (i % 8))) {  // 1: invalid
//			printf("[AT GC] invalid ppa: %d\n", i);
//			printf("[AT GC] bitmap: %d\n", bit_compare);
			invalid_cnt++; 
			if(invalid_cnt == _PPS) {	// all page is invalid

				__normal.li->trim_block(start_page_num, ASYNC);  //delete all segment
				return start_page_num; 
			}
			garbage_table[i/8] &= ~(1 << (i % 8));
		}
		else {//copy on reserved segment	// 0: valid
//			printf("[AT GC] valid ppa: %d\n", i);
			algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
			value_set *value_r = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);

			mapping_table[OOB[i]] = reserv_ppa_start;	// mapping_table update
			OOB[reserv_ppa_start] = OOB[i];			// OOB update
			OOB[i] = -1;
			garbage_table[reserv_ppa_start/8] &= ~(1 << (reserv_ppa_start % 8));

			//GCDR
			my_req->type = GCDR;
			my_req->end_req = pftl_end_req;
			gc_read_cnt++;

			__normal.li->read(i, PAGESIZE, value_r, ASYNC, my_req);
			
			//waiting for gc_read
			gc_general_waiting();
			
			//GCDW
			my_req = (algo_req *)malloc(sizeof(algo_req));
			my_req->type = GCDW;
			my_req->end_req = pftl_end_req;
			
			value_w = inf_get_valueset(value_r->value, FS_MALLOC_W, PAGESIZE);

			inf_free_valueset(value_r, FS_MALLOC_R);

			my_req->params = (void *)value_w;
			__normal.li->write(reserv_ppa_start, PAGESIZE, value_w, ASYNC, my_req);
			
			//increase reserved ppa number
			reserv_ppa_start++;
		}
	}
	
	/*      delete old segment	*/
	__normal.li->trim_block(start_page_num, ASYNC);
	
	// return update reserv segment number
	log_seg_num = start_page_num / _PPS;

	for(int i = start_page_num; i < end_page_num; i++) {
		garbage_table[i/8] &= ~(1 << (i % 8));
		if(OOB[i] != -1) {
			mapping_table[OOB[i]] = -1;
		}
	}

	return reserv_ppa_start;
}

void gc_general_waiting()
{
	//spin lcok for gc_read
	while(gc_read_cnt != gc_target_cnt) { }

	gc_read_cnt = 0;
	gc_target_cnt = 0;
}
