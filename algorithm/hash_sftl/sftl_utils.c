#include "dftl.h"


int32_t find_head_idx(uint32_t lpa){
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


int32_t set_entry(uint32_t lpa, int32_t ppa)
{

	C_TABLE *c_table = &CMT[D_IDX];
	hash_t *ht_ptr = c_table->ht_ptr;
	hash_node *f_node = NULL;
	bool *s_bitmap = c_table->s_bitmap;  //For sequentialty checking
	bool *d_bitmap = c_table->d_bitmap;  //For Entry set checking

	int32_t head_entry = ppa;
	int32_t head_lpn;
	int32_t old_ppa = -1;
	int32_t pre_ppa = -1;

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
			set_hash_entry(ht_ptr, lpa, head_entry);			
		}
		//Update head entry
		else{
			f_node = hash_lookup(ht_ptr, P_IDX);
			if(f_node != NULL){
				old_ppa = f_node->data;
				f_node->data = head_entry;
			}else{
				printf("Not found key error in first offset!\n");
				exit(0);
			}

		}
		//If Next entry is not allocated, return 0
		if(!d_bitmap[P_IDX+1])
			return 0;


		//Otherwise, add next entry on rb_tree for update entry
		if(old_ppa != -1){
			if(!s_bitmap[P_IDX+1]){
				set_hash_entry(ht_ptr, lpa+1, old_ppa+1);
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
				f_node = hash_lookup(ht_ptr, P_IDX);
				if(f_node != NULL){
					f_node->data = head_entry;
				}else{
					printf("Not found idx error in last offset\n");
					exit(0);
				}
			}	
			//Add head entry
			else{
				set_hash_entry(ht_ptr, lpa, head_entry);
			}
		//Otherwise, check previous ppa and current ppa
		}else{
			if(s_bitmap[P_IDX-1]){
				f_node = hash_lookup(ht_ptr, P_IDX-1);
				pre_ppa = f_node->data;
				if(head_entry == pre_ppa+1){
					if(s_bitmap[P_IDX]){
						free_hash_entry(ht_ptr, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						f_node = hash_lookup(ht_ptr, P_IDX);
						if(f_node != NULL){
							f_node->data = head_entry;
						}else{
							printf("Not found idx error in last offset!\n");
							exit(0);
						}
						
					}else{
						set_hash_entry(ht_ptr, lpa, head_entry);
					}
				}
			}else{
				head_lpn = find_head_idx(lpa-1);
				f_node = hash_lookup(ht_ptr, head_lpn);
				pre_ppa = get_entry(f_node, P_IDX-1); 

				if(head_entry == pre_ppa + 1){
					if(s_bitmap[P_IDX]){
						free_hash_entry(ht_ptr, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						f_node = hash_lookup(ht_ptr, P_IDX);
						if(f_node != NULL){
							f_node->data = head_entry;
						}else{
							printf("Not found idx error in last offset\n");
							exit(0);
						}
						
					}else{
						set_hash_entry(ht_ptr, lpa, head_entry);

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
				f_node = hash_lookup(ht_ptr, P_IDX);
				if(f_node != NULL){
					old_ppa = f_node->data;
					f_node->data = head_entry;
				}else{
					printf("Not found key error in middle offset\n");
					exit(0);
				}
			}else{
				set_hash_entry(ht_ptr, lpa, head_entry);
			}
		//Otherwise, check previous ppa and current ppa
		}else{
			if(s_bitmap[P_IDX-1]){
				f_node = hash_lookup(ht_ptr, P_IDX-1);
				if(f_node != NULL){
					pre_ppa = f_node->data;
					old_ppa = pre_ppa+1;
				}else{
					printf("Not found key error in middle offset!\n");
					exit(0);
				}

				if(head_entry == pre_ppa+1){
					if(s_bitmap[P_IDX]){
						old_ppa = free_hash_entry(ht_ptr, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						f_node = hash_lookup(ht_ptr, P_IDX);
						if(f_node != NULL){
							old_ppa = f_node->data;
							f_node->data = head_entry;
						}else{
							printf("Not found key error in middle offset!\n");
							exit(0);
						}
						
					}else{
						set_hash_entry(ht_ptr, lpa, head_entry);
					}
				}
			}else{

				head_lpn = find_head_idx(lpa-1);
				f_node = hash_lookup(ht_ptr, head_lpn);
				if(f_node != NULL){
					pre_ppa = get_entry(f_node, P_IDX-1);
					old_ppa = pre_ppa + 1;
				}else{
					printf("Not found key error in middle offset!\n");
					exit(0);
				}
				if(head_entry == pre_ppa + 1){
					if(s_bitmap[P_IDX]){
						old_ppa = free_hash_entry(ht_ptr, lpa);
					}
				}else{
					if(s_bitmap[P_IDX]){
						f_node = hash_lookup(ht_ptr, P_IDX);
						if(f_node != NULL){
							old_ppa = f_node->data;
							f_node->data = head_entry;
						}else{
							printf("Not found key error in middle offset!\n");
							exit(0);
						}
						
					}else{
						set_hash_entry(ht_ptr, lpa, head_entry);

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
				set_hash_entry(ht_ptr, lpa+1, old_ppa+1);
			}
		}



	}


	return 1;

}



int32_t get_entry(hash_node *f_node, int32_t offset){
	return (f_node->data + (offset - f_node->key));
}

void remove_entry(hash_t *ht_ptr){
	return hash_destroy(ht_ptr);
}

void set_hash_entry(hash_t *ht_ptr, uint32_t lpa, int32_t ppa){
	int32_t res;
	int32_t old_size = ht_ptr->size * P_SIZE;
	int32_t new_size = 0;
	C_TABLE *c_table = &CMT[D_IDX];
	res = hash_insert(ht_ptr, P_IDX, ppa);

	if(res == HASH_REBUILD){
		new_size = ht_ptr->size * P_SIZE;
		c_table->b_form_size -= old_size;
		c_table->b_form_size += new_size;
		c_table->b_form_size += NODE_SIZE;
	}else if(res == HASH_SUCCESS){
		c_table->b_form_size += NODE_SIZE;
	}else{
		c_table->b_form_size = PAGESIZE;
	}
	c_table->s_bitmap[P_IDX] = 1;
	c_table->bit_cnt++;

	return ;

}
int32_t free_hash_entry(hash_t *ht_ptr, uint32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	int32_t re_ppa;
	
	re_ppa = hash_delete(ht_ptr, P_IDX);
	if(re_ppa == HASH_FAIL){
		printf("Not found key error in free_hash_entry !\n");
		exit(0);
	}
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

int32_t reset_hash_entry(uint32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;
	bool *s_bitmap = c_table->s_bitmap;
	int32_t ppa;
	int32_t b_form_size = 0;
	int32_t bit_cnt = 0;



	for(int i = 0 ; i < EPP; i++){
		if(s_bitmap[i]){
			ppa = p_table[i].ppa;
			hash_insert(c_table->ht_ptr, i, ppa);
			b_form_size += NODE_SIZE;
			bit_cnt++;
		}
	}

	c_table->bit_cnt = bit_cnt;
	b_form_size += BITMAP_SIZE + (c_table->ht_ptr->size * P_SIZE);

	printf("[original]b_form_size = %d [reset]b_form_size = %d\n",b_form_size);
	if(c_table->b_form_size != b_form_size){
		printf("b_form_size different!\n");
		sleep(2);
	}

	return b_form_size;
}

int32_t cache_mapped_size()
{
        int32_t idx = max_cache_entry;
        int32_t cache_size = 0;
        int32_t cnt = 0;
        int32_t miss_cnt = 0;
        C_TABLE *c_table;
        D_TABLE *p_table;
        for(int i = 0; i < idx; i++)
        {
                c_table = &CMT[i];
                p_table = c_table->p_table;
                if(p_table)
                {
			printf("b_form_size[%d] = %d\n",i,c_table->b_form_size);
                        cnt++;
                }else{
                        miss_cnt++;
                }
        }
        cache_size = total_cache_size - free_cache_size;
        printf("not_caching : %d\n",miss_cnt);
        printf("num_caching : %d\n",cnt);
        return cache_size;


}


void hash_table_status(){
	C_TABLE *c_table = &CMT[2];
	hash_t *ht_ptr = c_table->ht_ptr;

	hash_buckets(ht_ptr);

	for(int i = 0 ; i < EPP; i++){
		printf("p_table[%d] = %d\n",i, c_table->p_table[i].ppa);
	}

	return ;
}

int32_t cached_entries(){
	C_TABLE *c_table;
	int32_t entries = 0;

	for(int i = 0 ; i < max_cache_entry; i++){
		c_table = &CMT[i];
		if(c_table->p_table){
			entries += c_table->ht_ptr->entries;
		}
	}

	return entries;
}





