#include <stdlib.h>
#include <stdio.h>
#include "gc.h"

extern struct algorithm algo_pftl;
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
	printf("[AT GC]---------------------------------\n");
	printf("[AT GC]---------------------------------\n");
	gc_read_cnt=0;
	gc_target_cnt=0;
	
	int start_page_num = erase_seg_num * _PPS;
	int end_page_num = (erase_seg_num+1) * _PPS;
	uint8_t bit_compare;

	
	printf("[AT GC] value_set front\n");
	/*            valid check & copy                */
	value_set *value=inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);

	printf("[AT GC] after value_set\n");
	for(uint32_t i = start_page_num; i < end_page_num; i++){ 	//valid checking
		printf("start loop! i: %d\n", i);
		algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
		
		if (garbage_table[i/8] & (1 << (i % 8))) {  // 1: invalid
			invalid_cnt++; 
			if(invalid_cnt == _PPS) {	// all page is invalid
				algo_pftl.li->trim_block(start_page_num, ASYNC);  //delete all segment
				return start_page_num; 
			}
		}
		else {//copy on reserved segment	// 0: valid
			mapping_table[OOB[i]] = reserv_ppa_start;	// mapping_table update
			OOB[reserv_ppa_start] = OOB[i];			// OOB update
			garbage_table[i/8] |= (1<<(i % 8));
			//GC_R
			my_req->type = GC_R;
			gc_read_cnt++;
			algo_pftl.li->read(i, PAGESIZE, value, 1, my_req);
			
			//waiting for gc_read
			printf("before general writing\n");
			gc_general_waiting();
			printf("after general writing\n");
			
			//GC_W
			my_req->type = GC_W;
			
			printf("before write\n");
			algo_pftl.li->write(reserv_ppa_start, PAGESIZE, value, 1, my_req);
			printf("after write\n");
			
			//increase reserved ppa number
			reserv_ppa_start++;
		}
	}
	
	printf("before trim\n");
	/*      delete old segment	*/
	algo_pftl.li->trim_block(start_page_num, ASYNC);
	printf("after trim\n");
	
	// return update reserv segment number
	log_seg_num = start_page_num / _PPS;

	for(int i=start_page_num; i<start_page_num+_PPS; i++){
		garbage_table[i/8] &= ~(1 << (i % 8));
	}
	printf("[AT GC] END OF GC\n");

	//TODO: free
//	inf_free_valueset(value, FS_MALLOC_R);
//	free(my_req);
	return reserv_ppa_start;
}

void gc_general_waiting()
{
	//spin lcok for gc_read
	while(gc_read_cnt != gc_target_cnt) { }

	gc_read_cnt = 0;
	gc_target_cnt = 0;
}
