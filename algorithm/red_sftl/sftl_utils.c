#include "dftl.h"


int32_t find_head_idx(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	int32_t re_idx = -1;
	for(int i = P_IDX ; i >= 0; i--){
		if(c_table->s_bitmap[i]){
			re_idx = i;
			return re_idx;
		}
	}

	printf("Not found head idx!\n");
    return re_idx;		
}


int32_t set_entry(int32_t lpa, int32_t ppa){

	C_TABLE *c_table = &CMT[D_IDX];
	Redblack root = c_table->rb_tree;
	Redblack f_node = NULL;
	bool *s_bitmap = c_table->s_bitmap;  //For sequentialty checking
	bool *d_bitmap = c_table->d_bitmap;  //For Entry set checking

	int32_t head_entry = ppa;
	int32_t head_lpn;
	int32_t old_ppa = -1;
	int32_t pre_ppa = -1;
	//int32_t next_ppa = -1;

	//Always set allocate bit for entry
	if(!d_bitmap[P_IDX])
		d_bitmap[P_IDX] = 1;
	//CASE 1 : First offset
	/*
	   --------------------
	        | O | X | X |
     P_IDX:   F   M   L
	  ---------------------
	*/
	if(P_IDX == 0){
		//Init head entry
		if(!s_bitmap[P_IDX]){
			set_rb_entry(root, lpa, head_entry);			
		}
		//Update head entry
		else{
			if(rb_find_int(root, P_IDX, &f_node)){
				old_ppa = f_node->h_ppa;
				f_node->h_ppa = head_entry;
			}else{
				printf("Not found idx error!\n");
				exit(0);
			}
		}
		//If Next entry is not allocated, return 0
		if(!d_bitmap[P_IDX+1])
			return 0;


		//Otherwise, add next entry on rb_tree for update entry
		if(old_ppa != -1){
			if(!s_bitmap[P_IDX+1]){
				set_rb_entry(root, lpa+1, old_ppa+1);
			}
		}
	
	}
	//CASE 2 : Last offset
    /*
	   ----------------------
	        | X | X | O |
     P_IDX:   F   M   L
	  -----------------------
	*/	
	else if(P_IDX == EPP-1){
		//If pre_ppa is not allocated, Only set current entry
		if(!d_bitmap[P_IDX-1]){
			//Update head entry
			if(s_bitmap[P_IDX]){
				if(rb_find_int(root, P_IDX, &f_node)){
					f_node->h_ppa = head_entry;
				}else{
					printf("Not found idx error in last offset\n");
					exit(0);
				}
			//Add head entry
			}else{
				set_rb_entry(root, lpa, head_entry);
			}
		//Otherwise, check previous ppa and current ppa
		}else{
			if(s_bitmap[P_IDX-1]){
				rb_find_int(root, P_IDX-1, &f_node);
				pre_ppa = f_node->h_ppa;
				if(head_entry == pre_ppa+1){
					if(s_bitmap[P_IDX]){
						free_rb_entry(root, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						if(rb_find_int(root, P_IDX, &f_node)){
							f_node->h_ppa = head_entry;
						}else{
							printf("Not found idx error in last offset\n");
							exit(0);
						}
					}else{
						set_rb_entry(root, lpa, head_entry);
					}
				}
			}else{
				head_lpn = find_head_idx(lpa-1);
				rb_find_int(root, head_lpn, &f_node);
				pre_ppa = get_entry(f_node, P_IDX-1);
				if(head_entry == pre_ppa + 1){
					if(s_bitmap[P_IDX]){
						free_rb_entry(root, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						if(rb_find_int(root, P_IDX, &f_node)){
							f_node->h_ppa = head_entry;
						}else{
							printf("Not found idx error in last offset\n");
							exit(0);
						}
					}else{
						set_rb_entry(root, lpa, head_entry);

					}
				}
			}

		}

	}
	//CASE 3 : Middle offset
    /*
	   ----------------------
	        | X | O | X |
     P_IDX:   F   M   L
	  -----------------------
	*/	
	else{
		//If pre_ppa is not allocated, Only set current entry
		if(!d_bitmap[P_IDX-1]){
			if(s_bitmap[P_IDX]){
				if(rb_find_int(root, P_IDX, &f_node)){
					old_ppa = f_node->h_ppa;
					f_node->h_ppa = head_entry;
				}else{
					printf("Not found idx error in last offset\n");
					exit(0);
				}
			}else{
				set_rb_entry(root, lpa, head_entry);
			}
		//Otherwise, check previous ppa and current ppa
		}else{
			if(s_bitmap[P_IDX-1]){
				rb_find_int(root, P_IDX-1, &f_node);
				pre_ppa = f_node->h_ppa;
				old_ppa = pre_ppa + 1;
				if(head_entry == pre_ppa+1){
					if(s_bitmap[P_IDX]){
						old_ppa = free_rb_entry(root, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						if(rb_find_int(root, P_IDX, &f_node)){
							old_ppa = f_node->h_ppa;	
							f_node->h_ppa = head_entry;
						}else{
							printf("Not found idx error in last offset\n");
							exit(0);
						}
					}else{
						set_rb_entry(root, lpa, head_entry);
					}
				}
			}else{
				head_lpn = find_head_idx(lpa-1);
				rb_find_int(root, head_lpn, &f_node);
				pre_ppa = get_entry(f_node, P_IDX-1);
				old_ppa = pre_ppa + 1;
				if(head_entry == pre_ppa + 1){
					if(s_bitmap[P_IDX]){
						old_ppa = free_rb_entry(root, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						if(rb_find_int(root, P_IDX, &f_node)){
							old_ppa = f_node->h_ppa;
							f_node->h_ppa = head_entry;
						}else{
							printf("Not found idx error in last offset\n");
							exit(0);
						}
					}else{
						set_rb_entry(root, lpa, head_entry);

					}
				}
			}

		}


		//If next entry is not allocated, return 0
		if(!d_bitmap[P_IDX+1])
			return 0;

		//Otherwise, check next entry and current entry
		if(old_ppa != -1){
			if(!s_bitmap[P_IDX+1]){
				set_rb_entry(root, lpa+1, old_ppa+1);
			}
		}



	}


	return 1;

		

}

int32_t get_entry(Redblack f_node, int32_t offset){
	return (f_node->h_ppa + (offset - f_node->key));
}

void remove_entry(Redblack root){
	return rb_clear(root);
}

void set_rb_entry(Redblack root, int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	rb_insert_int(root, P_IDX, ppa);
	
	c_table->s_bitmap[P_IDX] = 1;
	c_table->b_form_size += NODE_SIZE;
	c_table->bit_cnt++;

	return ;
}
int32_t free_rb_entry(Redblack root, int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	Redblack d_node;
	int32_t re_ppa;

	rb_find_int(root, P_IDX, &d_node);
	re_ppa = d_node->h_ppa;
	rb_delete(d_node);
	c_table->s_bitmap[P_IDX] = 0;
	c_table->b_form_size -= NODE_SIZE;
	c_table->bit_cnt--;

	return re_ppa;
}

int32_t reset_bitmap(int32_t t_index){
	C_TABLE *c_table = &CMT[t_index];
	D_TABLE *p_table = mem_arr[t_index].mem_p;

	int32_t b_form_size = 0;	
	int32_t bit_cnt = 0;
	int32_t head_ppa, next_ppa;
	int32_t start_idx;
	
	for(int i = 0 ; i < EPP-1; i++){
		if(p_table[i].ppa != -1){
			start_idx = i;
			head_ppa = p_table[i].ppa;
			c_table->s_bitmap[i] = 1;
			bit_cnt++;
			break;
		}else{
			c_table->s_bitmap[i] = 0;
		}
	}

	for(int i = start_idx; i < EPP-1; i++){
		next_ppa = p_table[i+1].ppa;
		if(next_ppa == -1){
			c_table->s_bitmap[i+1] = 0;
			continue;
		}

		if(next_ppa == head_ppa + 1){
			c_table->s_bitmap[i+1] = 0;
		}else{
			bit_cnt++;
			c_table->s_bitmap[i+1] = 1;
		}
		head_ppa = next_ppa;

	}

	b_form_size = (bit_cnt * NODE_SIZE) + BITMAP_SIZE;
	c_table->bit_cnt = bit_cnt;
	return b_form_size;

	
}

int32_t reset_rb_entry(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLe *p_table = c_table->p_table;
	bool *s_bitmap = c_table->s_bitmap;
	int32_t ppa;
	int32_t b_form_size = 0;
	int32_t bit_cnt = 0;

	for(int i = 0 ; i < EPP; i++){
		if(s_bitmap[i]){
			ppa = p_table[i].ppa;
			rb_insert_int(c_table->rb_tree, i, ppa);
			b_form_size += NODE_SIZE;
			bit_cnt++;
		}
	}

	c_table->bit_cnt = bit_cnt;
	b_form_size += BITMAP_SIZE;

	return b_form_size;
}





