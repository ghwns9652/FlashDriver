#include "dftl.h"


int32_t find_head_idx(int32_t lpa, int32_t p_idx){
	
	int32_t re_idx = -1;
	for(int i = p_idx ; i >= 0 i--){
		if(c_table->bitmap[i]){
			re_idx = i;
			return re_idx;
		}
	}

	printf("Not found head idx!\n");
        return re_idx;		
}


int32_t entry_set(int32_t lpa, int32_t ppa){

	C_TABLE *c_table = &CMT[D_IDX];
	Redblack root = c_table->rb_tree;
	Redblack f_node = NULL;
	bool *s_bitmap = c_table->s_bitmap;  //For sequentialty checking
	bool *d_bitmap = c_table->d_bitmap;  //For Entry set checking

	int32_t head_entry = ppa;
	int32_t old_ppa;

	if(!d_bitmap[P_IDX])
		d_bitmap[P_IDX] = 1;
	//CASE 1 : First offset
	if(P_IDX == 0){
		//Init head entry
		if(!s_bitmap[P_IDX]){
			s_bitmap[P_IDX] = 1;
			rb_insert_int(root, P_IDX, ppa);
		}
		//Update head entry
		else{
			if(rb_find_int(root, P_IDX, &f_node)){
				old_ppa = f_node->h_ppa;
				f_node->h_ppa = ppa;
			}else{
				printf("Not found idx error!\n");
				exit(0);
			}
		}
		//If Next entry is not allocate, return 0
		if(!d_bitmap[P_IDX+1])
			return 0;

		//Otherwise, add next entry on rb_tree
		if(!s_bitmap[P_IDX+1]){
			s_bitmap[P_IDX+1] = 1;
			rb_insert_int(root, P_IDX, old_ppa+1);
		}
	}
	//CASE 2 : Last offset
	else if(P_IDX == EPP-1){

	}
	//CASE 3 : Middle offset
	else{
	}


	return 1;

		

}
