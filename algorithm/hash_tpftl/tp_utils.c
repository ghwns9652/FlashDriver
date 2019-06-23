#include "dftl.h"

int32_t find_head_idx(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	int32_t re_idx = -1;
	for(int i = P_IDX; i >= 0; i--){
		if(c_table->s_bitmap[i]){
			re_idx = i;
			return re_idx;
		}
	}
	printf("Not found head idx!\n");
	return re_idx;

}

void tp_init_hash(C_TABLE *c_table){
	
	hash_init(c_table->ht_ptr, BUCKET_SIZE);
	free_cache_size -= BITMAP_SIZE;
	free_cache_size -= c_table->ht_ptr->size * P_SIZE;
	return ;
}

hash_node *tp_entry_search(int32_t lpa, int32_t offset){
	C_TABLE *c_table = &CMT[D_IDX];
	//hash_node *re;

	//If LRU pointer is null, return null
	if(c_table->queue_ptr == NULL){
		return NULL;
	}else{
		//If node already exists, return node. Otherwise return null
		return hash_lookup(c_table->ht_ptr, offset);
	}

}


hash_node *tp_entry_alloc(int32_t lpa, int32_t offset, int32_t ppa, int8_t cnt, char req_t){	

	C_TABLE *c_table = &CMT[D_IDX];
	hash_node *node = NULL;
	int32_t old_size = c_table->ht_ptr->size;
	int32_t new_size;

	node = hash_insert(c_table->ht_ptr, offset, ppa, cnt);
	new_size = c_table->ht_ptr->size;
	if(new_size != old_size){  //If rebuild operation is not working, Only calculate for node size
		free_cache_size += (old_size * P_SIZE);
		free_cache_size -= (new_size * P_SIZE);
	}
	if(req_t == 'W')
		node->state = DIRTY;
	else
		node->state = CLEAN;
	c_table->s_bitmap[offset] = 1;
	c_table->entry_cnt++;
	free_cache_size -= NODE_SIZE;
	return node;
}
void tp_entry_free(int32_t lpa, hash_node *d_node){
	C_TABLE *c_table = &CMT[D_IDX];
	int32_t re_ppa;

	c_table->s_bitmap[d_node->key] = 0;
	re_ppa = hash_delete(c_table->ht_ptr, d_node);
	if(re_ppa == HASH_FAIL){
		printf("Not found key error in tp_entry_free!\n");
		exit(0);
	}else{
		c_table->entry_cnt--;
		free_cache_size += NODE_SIZE;
	}	
	return ;
}

hash_node *tp_entry_update(hash_node* update_node, int32_t lpa, int32_t offset, int32_t ppa, int8_t cnt, char req_t){
	C_TABLE *c_table = &CMT[D_IDX];
	hash_node *old = update_node;
	hash_node *now;

	//Delete old node after update old mapping information
	c_table->s_bitmap[old->key] = 0;
	hash_update_delete(c_table->ht_ptr,old);
	
	//Insert new hash node
	now = hash_insert(c_table->ht_ptr, offset, ppa, cnt);
	c_table->s_bitmap[now->key] = 1;
	if(req_t == 'W')
		now->state = DIRTY;
	else
		now->state = CLEAN;
	//free(old);

	return now;
}

hash_node *tp_entry_op(int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	hash_node *last_ptr = c_table->last_ptr;
	hash_node *find_node = NULL;	
	//hash_node * now       = NULL;
	int32_t index      = -1;	
	int32_t pre_ppa    = -1;
	int32_t head_idx   = last_ptr->key;
	int32_t head_ppa   = last_ptr->data;
	int32_t head_cnt   = last_ptr->cnt;
	int32_t max_range  = head_idx + head_cnt;
	index   = head_idx + head_cnt + 1;
	pre_ppa = head_ppa + (P_IDX - head_idx - 1);

	int32_t b_align;
	int32_t f_offset;
	bool merge_flag = 0;
	bool alloc_flag = 0;
//	printf("lpa = %d ppa : %d P_IDX : %d\n",lpa, ppa, P_IDX);
//	printf("last_ptr->key : %d last_ptr->ppa : %d\n",last_ptr->key, last_ptr->data);
	if(index == P_IDX && ppa == pre_ppa+1){
		b_align = pre_ppa % _PPS + 1;
		
		//If head_cnt is full, create new entry node
		if(head_cnt == MAX_CNT || b_align == _PPS){
			alloc_flag = 1;
		}else{
			merge_flag = 1;
		}
		if(c_table->h_bitmap[P_IDX]){
			f_offset = find_head_idx(lpa);
			find_node = tp_entry_search(lpa, f_offset);
		}

		if(find_node != NULL){
				tp_entry_split(find_node, lpa, ppa, 0);
		}
		if(alloc_flag){
			last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'W');
		}
		if(merge_flag){
			last_ptr->cnt++;
		}
	

	}else{
	
		if(head_idx <= P_IDX && P_IDX <= max_range){
			last_ptr = tp_entry_split(last_ptr, lpa, ppa, 1);
		}else{
			if(c_table->h_bitmap[P_IDX]){
				f_offset = find_head_idx(lpa);
				find_node = tp_entry_search(lpa, f_offset);
			}
			if(find_node != NULL){
				last_ptr = tp_entry_split(find_node, lpa, ppa, 1);
			}else{
				last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'W');
			}
		}
	}
	return last_ptr;
	
}
hash_node *tp_entry_split(hash_node *split_node, int32_t lpa, int32_t ppa, bool flag){
	C_TABLE *c_table  = &CMT[D_IDX];
	hash_node *last_ptr = NULL;
	int32_t head_lpn  = split_node->key;
	int32_t head_ppn  = split_node->data;
	int32_t cnt       = split_node->cnt;
	int32_t distance  = 0;
	int32_t pre_cnt, next_cnt;
	if(P_IDX == head_lpn){
		//Split After merge	
		if(!flag){
			if(cnt == 0){
				tp_entry_free(lpa, split_node);
				return c_table->last_ptr;
			}
			last_ptr = tp_entry_update(split_node,lpa, head_lpn+1, head_ppn+1, split_node->cnt-1,'W');
			
		}
		//Just split	
		else{
			if(cnt == 0){
				last_ptr = tp_entry_update(split_node,lpa, P_IDX, ppa, split_node->cnt,'W');
				return last_ptr;
			}
			tp_entry_update(split_node,lpa, head_lpn+1, head_ppn+1, split_node->cnt-1,'W');
			last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0,'W');
		}

	}
	else if (P_IDX < head_lpn + cnt){

		distance = P_IDX - head_lpn;
		pre_cnt  = (distance) - 1;
		next_cnt = (cnt - distance) - 1;
		//next_node insert
		tp_entry_update(split_node, lpa, P_IDX+1, head_ppn+distance+1, next_cnt,'W');
		//middle_node insert
		last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0,'W');
		//first_node insert
		tp_entry_alloc(lpa, head_lpn, head_ppn, pre_cnt, 'W');
//		cache_single_entries(c_table);

	}
	else{

		if(cnt == 0){
			tp_entry_free(lpa, split_node);
			return last_ptr;
		}
		tp_entry_update(split_node, lpa, head_lpn, head_ppn, split_node->cnt-1, 'W');
		last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'W');
	}
	return last_ptr;
}

hash_node* hash_eviction(C_TABLE *evic_ptr, int32_t lpa, bool flag){
	C_TABLE *c_table = &CMT[D_IDX];
	hash_t *ht_ptr = evic_ptr->ht_ptr;
	hash_node *evic_node;
	int32_t idx = 0;
	int32_t evic_idx = evic_ptr->evic_idx;
	int32_t p_idx;
	int32_t start, end;

	int32_t h;

	for(int i = 0 ; i < ht_ptr->size; i++){
		if(ht_ptr->bucket[evic_idx] != NULL){
			break;
		}
		evic_idx = (evic_idx+1) % ht_ptr->size;
		evic_ptr->evic_idx = evic_idx;
	}


	//Select evic node, which is bucket head node
	evic_node = ht_ptr->bucket[evic_idx];
	if(evic_node == NULL){
		printf("evic_idx = %d\n",evic_idx);
		printf("entries : %d\n",evic_ptr->ht_ptr->entries);
		cache_single_entries(evic_ptr);
		printf("EVIC_NODE IS NULL!\n");
		exit(0);
	}
	//Switch head node
	//printf("evic_idx = %d\n",evic_idx);
	ht_ptr->bucket[evic_idx] = evic_node->next;
	
	//Reset h_bitmap and s_bitmap	
	p_idx = start = evic_node->key;
	end = start + evic_node->cnt;
	evic_ptr->s_bitmap[p_idx] = 0;
	
	for(int32_t i = start; i <= end; i++){
		evic_ptr->h_bitmap[i] = 0;
	}
	ht_ptr->bucket_cnt[evic_idx]--;
	ht_ptr->entries--;
	evic_ptr->entry_cnt--;
	//Reset count variable

	if(flag){
		c_table->flying_mapping_size = NODE_SIZE;
	}else{
		free_cache_size += NODE_SIZE;
	}
	//Reset evic_idx
	/*
	if(ht_ptr->bucket[evic_idx] == NULL){	
		evic_idx = (evic_idx+1) % ht_ptr->size;
		evic_ptr->evic_idx = evic_idx;
		if(evic_ptr->evic_idx == ht_ptr->size-1)
			evic_ptr->evic_idx = 0;
	}
	*/
	return evic_node;

}



void tp_batch_update(C_TABLE *evic_ptr){
	hash_t *ht_ptr = evic_ptr->ht_ptr;
	hash_node *node;
	for(int i = 0; i < ht_ptr->size; i++){
		node = ht_ptr->bucket[i];
		while(node != NULL){
			if(node->state == DIRTY)
				node->state = CLEAN;
			node = node->next;
		}
	}
	return ;

}


hash_node *tp_fetch(int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	hash_node *last_ptr = c_table->last_ptr;
	hash_node *pre_find = NULL;
	hash_node *next_find = NULL;
	int32_t pre_ppa, cur_ppa, next_ppa;
	int32_t cnt = 0;
	int32_t merge_cnt = 0;
	int32_t next_cnt = 0;
	int32_t i;
	int32_t idx;
	int32_t f_offset;
	if(ppa == -1){
		return NULL;
	}
	if(P_IDX == 0){
		next_ppa = c_table->p_table[P_IDX+1].ppa;
		if(next_ppa == -1 || next_ppa != ppa+1){
			last_ptr =  tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
			c_table->h_bitmap[P_IDX] = 1;
			return last_ptr;
		}
		if(next_ppa == ppa+1){
			if(c_table->h_bitmap[P_IDX+1]){
				if(c_table->s_bitmap[P_IDX+1]){
					next_find = tp_entry_search(lpa, P_IDX+1);
				}
			}

			//If next_idx is hit
			if(next_find != NULL){
				if(next_find->cnt == MAX_CNT){
					last_ptr =  tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
				}else{
					last_ptr = tp_entry_update(next_find, lpa, P_IDX, ppa, next_find->cnt+1, 'R');
				}
				c_table->h_bitmap[P_IDX] = 1;
			}else{
				cur_ppa = ppa;
				c_table->h_bitmap[P_IDX] = 1;
				for(i = P_IDX+1; i < EPP; i++){
					if(c_table->h_bitmap[i] || cnt == MAX_CNT)
						break;
					next_ppa = c_table->p_table[i].ppa;
					if(next_ppa == cur_ppa+1){
						cnt++;
						c_table->h_bitmap[i] = 1;
						cur_ppa = next_ppa;
					}else{
						break;
					}
				}		
				last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, cnt, 'R');
			}
		}
		
	}
	//0 < P_IDX < EPP
	else if (P_IDX < EPP-1){
	      	
		pre_ppa = c_table->p_table[P_IDX-1].ppa;
		next_ppa = c_table->p_table[P_IDX+1].ppa;

		if(pre_ppa == -1 || ppa != pre_ppa+1){
			last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
			c_table->h_bitmap[P_IDX] = 1;
		}
		else if (ppa == pre_ppa+1){
			if(c_table->h_bitmap[P_IDX-1]){
				if(c_table->s_bitmap[P_IDX-1]){
					pre_find = tp_entry_search(lpa, P_IDX-1);
				}else{
					f_offset = find_head_idx(lpa-1);
					pre_find = tp_entry_search(lpa, f_offset);
				}
			}

			//Merge request into pre_node
			if(pre_find != NULL){
				//Increment count variable
				if(pre_find->cnt == MAX_CNT){
					last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
				}else{
					pre_find->cnt++;
					last_ptr = pre_find;
				}
				c_table->h_bitmap[P_IDX] = 1;
			}else{
				cur_ppa = ppa;
				c_table->h_bitmap[P_IDX] = 1;	
				for(i = P_IDX-1; i >= 0; i--){
					if(c_table->h_bitmap[i] || cnt == MAX_CNT)
						break;
					pre_ppa = c_table->p_table[i].ppa;
					if(cur_ppa == pre_ppa +1){
						cnt++;
						c_table->h_bitmap[i] = 1;
					}else{
						break;
					}
					cur_ppa = pre_ppa;
				}
				if(i != -1){
					idx = i+1;
				}else{
					idx = 0;
				}	
				last_ptr = tp_entry_alloc(lpa, idx, cur_ppa, cnt, 'R');
			}
		}

		if(next_ppa == -1 || next_ppa != ppa+1 || last_ptr->cnt == MAX_CNT){
			return last_ptr;
		}
		else if(next_ppa == ppa+1){

			if(c_table->h_bitmap[P_IDX+1]){
				if(c_table->s_bitmap[P_IDX+1]){
					next_find = tp_entry_search(lpa, P_IDX+1);
				}else{
					f_offset = find_head_idx(lpa+1);
					next_find = tp_entry_search(lpa, f_offset);
				}
			}
			//CASE1 : merge pre_node and next_node
			if(next_find != NULL){
				next_cnt = next_find->cnt;
				for(int i = 0 ; i <= next_cnt; i++){
					last_ptr->cnt++;
					next_find->key++;
					next_find->data++;
					next_find->cnt--;
					if(last_ptr->cnt == MAX_CNT)
						break;
				}
				if(next_find->cnt == -1)
				{
					tp_entry_free(lpa, next_find);
				}
			}
			//CASE2 : next_find is null
			else
			{

				cur_ppa = ppa;
				merge_cnt = last_ptr->cnt;
				//P_IDX bitmap is already set
				//c_table->h_bitmap[P_IDX] = 1
				for(i = P_IDX+1 ; i < EPP; i++){
					if(c_table->h_bitmap[i] || merge_cnt == MAX_CNT)
						break;
					next_ppa = c_table->p_table[i].ppa;
					if(next_ppa == cur_ppa + 1){
						merge_cnt++;
						c_table->h_bitmap[i] = 1;
						cur_ppa = next_ppa;
					}else{
						break;
					}
				}
				
				last_ptr->cnt = merge_cnt;


			}

			
		}			

	}
	else{
		pre_ppa = c_table->p_table[P_IDX-1].ppa;
		if(pre_ppa == -1 || ppa != pre_ppa+1){
			last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
			c_table->h_bitmap[P_IDX] = 1;
			return last_ptr;
		}

		if(ppa == pre_ppa+1){
			if(c_table->h_bitmap[P_IDX-1]){
				if(c_table->s_bitmap[P_IDX-1]){
					pre_find = tp_entry_search(lpa, P_IDX-1);
				}else{
					f_offset = find_head_idx(lpa-1);
					pre_find = tp_entry_search(lpa, f_offset);
				}
			}
			if(pre_find != NULL)
			{
				if(pre_find->cnt == MAX_CNT){
					last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
				}else{
					pre_find->cnt++;
					last_ptr = pre_find;
				}
				c_table->h_bitmap[P_IDX] = 1;
			}else{
				cur_ppa = ppa;
				c_table->h_bitmap[P_IDX] = 1;
				for(i = P_IDX-1; i >= 0; i--){
					if(c_table->h_bitmap[i] || cnt == MAX_CNT)
						break;
					pre_ppa = c_table->p_table[i].ppa;
					if(cur_ppa == pre_ppa +1){
						cnt++;
						c_table->h_bitmap[i] = 1;
					}else{
						break;
					}
					cur_ppa = pre_ppa;
				}
				if(i != -1){
					idx = i+1;
				}else{
					idx = 0;
				}
				last_ptr = tp_entry_alloc(lpa, idx, cur_ppa, cnt, 'R');
			}
		}
		
	}

	return last_ptr;
}
int32_t cache_print_entries(){
	C_TABLE *c_table;
	hash_t *ht_ptr;
	hash_node *node;
	for(int i = 0 ; i < max_cache_entry; i++){
		c_table = &CMT[i];
		if(c_table->queue_ptr){
			ht_ptr = c_table->ht_ptr;
			for(int i = 0 ; i < ht_ptr->size; i++){
				node = ht_ptr->bucket[i];
				while(node != NULL){
					printf("node_key : %d ppa : %d cnt : %d\n",node->key, node->data, node->cnt);
					node = node->next;
				}
			}
		}
	}
	return 1;
}

int32_t cache_single_entries(C_TABLE *c_table){
	hash_t *ht_ptr = c_table->ht_ptr;
	hash_node *node;

	for(int i = 0 ; i < ht_ptr->size; i++){
		node = ht_ptr->bucket[i];
		while(node != NULL){
			printf("key : %d ppa : %d cnt : %d\n",node->key, node->data, node->cnt);
			node = node->next;
		}
	}

	return 1;
}



int32_t cached_entries(){
	C_TABLE *c_table;
	for(int i = 0 ; i < max_cache_entry; i++){
		c_table = &CMT[i];
		if(c_table->queue_ptr){
			printf("CMT[%d]-->entries : %d\n",i,c_table->entry_cnt);
		}
	}

	return 1;
}



