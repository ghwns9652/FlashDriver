#include "dftl.h"

Redblack tp_entry_search(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	Redblack re;

	//If LRU pointer is null, return null
	if(c_table->queue_ptr == NULL){
		return NULL;
	}

	//If node already exists, return node. Otherwise return null
	if(rb_find_int(c_table->rb_tree, P_IDX, &re))
		return re;
	else
		return NULL;
}


Redblack tp_entry_alloc(int32_t lpa, int32_t offset, int32_t ppa, int8_t cnt, char req_t){	

	C_TABLE *c_table = &CMT[D_IDX];
	Redblack now = rb_insert_int(c_table->rb_tree, offset, ppa, cnt);
	if(req_t == 'W')
		now->state = DIRTY;
	else
		now->state = CLEAN;
	c_table->entry_cnt++;
	free_cache_size -= ENTRY_SIZE;

	return now;
}
void tp_entry_free(int32_t lpa, Redblack d_node){
	C_TABLE *c_table = &CMT[D_IDX];

	rb_delete(d_node);
	c_table->entry_cnt--;
	free_cache_size += ENTRY_SIZE;
	
	return ;
}

Redblack tp_entry_update(Redblack update_node, int32_t offset, int32_t ppa, int8_t cnt, char req_t){
	update_node->key   = offset;
	update_node->h_ppa = ppa;
	update_node->cnt   = cnt;
	if(req_t == 'W')
		update_node->state = DIRTY;
	else
		update_node->state = CLEAN;

	return update_node;
}

Redblack tp_entry_op(int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	Redblack last_ptr = c_table->last_ptr;
	Redblack find_node = NULL;	
	Redblack now       = NULL;
	int32_t index      = -1;	
	int32_t pre_ppa    = -1;
	int32_t head_idx   = last_ptr->key;
	int32_t head_ppa   = last_ptr->h_ppa;
	int32_t head_cnt   = last_ptr->cnt;
	int32_t max_range  = head_idx + head_cnt;
	index   = head_idx + head_cnt + 1;
	pre_ppa = head_ppa + (P_IDX - head_idx - 1);

	int32_t b_align;
	bool merge_flag = 0;
	bool alloc_flag = 0;
	//printf("last_ptr->key : %d last_ptr->ppa : %d\n",last_ptr->key, last_ptr->h_ppa);
	if(index == P_IDX && ppa == pre_ppa+1){
		b_align = pre_ppa % _PPS + 1;
		
		//If head_cnt is full, create new entry node
		if(head_cnt == MAX_CNT || b_align == _PPS){
			alloc_flag = 1;
		}else{
			merge_flag = 1;
		}
#if B_TPFTL
		if(c_table->h_bitmap[P_IDX])
			find_node = tp_entry_search(lpa);

#else
		find_node = tp_entry_search(lpa);
#endif
		if(find_node != NULL){
	//		printf("lpa  : %d\t P_IDX : %d\t",lpa,P_IDX);
	//		printf("find_key : %d ppa : %d\n",find_node->key, find_node->h_ppa);
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
#if B_TPFTL
			if(c_table->h_bitmap[P_IDX])
				find_node = tp_entry_search(lpa);
			
#else
			find_node = tp_entry_search(lpa);
#endif
			if(find_node != NULL){
				last_ptr = tp_entry_split(find_node, lpa, ppa, 1);
			}else{
				last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'W');
			}
		}
	}
	return last_ptr;
	
}
Redblack tp_entry_split(Redblack split_node, int32_t lpa, int32_t ppa, bool flag){
	C_TABLE *c_table  = &CMT[D_IDX];
	Redblack last_ptr = NULL;
	int32_t head_lpn  = split_node->key;
	int32_t head_ppn  = split_node->h_ppa;
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

			last_ptr = tp_entry_update(split_node, head_lpn+1, head_ppn+1, split_node->cnt-1,'W');
			
		}
		//Just split	
		else{
			if(cnt == 0){
				last_ptr = tp_entry_update(split_node, P_IDX, ppa, split_node->cnt, 'W');
				return last_ptr;
			}
			tp_entry_update(split_node, head_lpn+1, head_ppn+1, split_node->cnt-1, 'W');
			last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0,'W');
		}

	}
	else if (P_IDX < head_lpn + cnt){

		distance = P_IDX - head_lpn;
		pre_cnt  = (distance) - 1;
		next_cnt = (cnt - distance) - 1;
		//next_node insert
		tp_entry_update(split_node, P_IDX+1, head_ppn+distance+1, next_cnt,'W');
		//middle_node insert
		last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0,'W');
		//first_node insert
		tp_entry_alloc(lpa, head_lpn, head_ppn, pre_cnt, 'W');


	}
	else{
		/*
		if(cnt == 0){
			tp_entry_free(lpa, split_node);
			//return c_table->last_ptr;
			return last_ptr;
		}
		*/
		tp_entry_update(split_node, head_lpn, head_ppn, split_node->cnt-1,'W');
		last_ptr = tp_entry_alloc(lpa, P_IDX, ppa, 0, 'W');
	}
	return last_ptr;
}
void tp_batch_update(C_TABLE *evic_ptr, int32_t lpa, bool flag){
	C_TABLE *c_table = &CMT[D_IDX];
	Redblack root = evic_ptr->rb_tree;
	Redblack node = rb_first(root), next;
	int32_t start, end;

	assert(root == root->left);


	while(node != root){
		next  = rb_next(node);
		start = node->key;
		end   = start + node->cnt;
		for(int32_t i = start ; i <= end; i++){
			evic_ptr->h_bitmap[i] = 0;
		}
		rb_delete(node);
		evic_ptr->entry_cnt--;
		//Normal eviction
		if(flag){
			c_table->flying_mapping_size += ENTRY_SIZE;
		}else{
			free_cache_size += ENTRY_SIZE;
		}
		node = next;
	}
#ifdef RB_LINK
	root->next = root->prev =
#endif
		root->left = root->right = root;

}



/*
NODE* tp_get_entry(int32_t lpa, int32_t offset){
	C_TABLE *c_table = &CMT[D_IDX];
	struct entry_node *ent_node = NULL;
	NODE *find_node = NULL;
	if(c_table->queue_ptr == NULL)
		return NULL;
	// variable for debug
	
	int32_t head_ppn;
	int32_t head_lpn;
	int32_t ppa;	
	int32_t cnt = 0;
	
	while(entry_node != NULL){

		ent_node = (struct entry_node*)entry_node->DATA;
		if(ent_node->p_index <= offset && offset <= ent_node->p_index + ent_node->cnt){
			
			head_lpn = ent_node->p_index;
			head_ppn = ent_node->ppa;
			int32_t distance = P_IDX - head_lpn;
			ppa = head_ppn + distance;
			
			//printf("D_IDX = %d %d = [%d | %d | %d]\n",D_IDX,P_IDX, head_lpn, head_ppn, ent_node->cnt);
			find_node = entry_node;
			
			//cnt++;
			if(cnt > 1){
				printf("Double node!!\n");
				for(int i = 0 ; i < EPP ;i++){
					printf("[%d] = %d\n",i,c_table->p_table[i].ppa);
					//printf("p_table[%d].bitmap = %d\n",i,c_table->h_bitmap[i]);
				}
				exit(0);
			}
			return find_node;
			

		}

		entry_node = entry_node->next;
	}

	return find_node;
}
*/
Redblack tp_fetch(int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	Redblack last_ptr = c_table->last_ptr;
	Redblack pre_find = NULL;
	Redblack next_find = NULL;
	int32_t pre_ppa, cur_ppa, next_ppa;
	int32_t cnt = 0;
	int32_t merge_cnt = 0;
	int32_t next_cnt = 0;
	int32_t i;
	int32_t idx;
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
			if(c_table->h_bitmap[P_IDX+1])
				next_find = tp_entry_search(lpa+1);
			//If next_idx is hit
			if(next_find != NULL){
				if(next_find->cnt == MAX_CNT){
					last_ptr =  tp_entry_alloc(lpa, P_IDX, ppa, 0, 'R');
				}else{
					last_ptr = tp_entry_update(next_find, P_IDX, ppa, next_find->cnt+1,'R');
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
			if(c_table->h_bitmap[P_IDX-1])
				pre_find = tp_entry_search(lpa-1);
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

			if(c_table->h_bitmap[P_IDX+1])
				next_find = tp_entry_search(lpa+1);
			//CASE1 : merge pre_node and next_node
			if(next_find != NULL){
				next_cnt = next_find->cnt;
				for(int i = 0 ; i <= next_cnt; i++){
					last_ptr->cnt++;
					next_find->key++;
					next_find->h_ppa++;
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

			if(c_table->h_bitmap[P_IDX-1])
				pre_find = tp_entry_search(lpa-1);
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
	Redblack root;
	Redblack node, next;
	int32_t start, end;



	for(int i = 0 ; i < max_cache_entry; i++){
		c_table = &CMT[i];
		if(c_table->queue_ptr){

			root = c_table->rb_tree;
			node = rb_first(root);
			assert(root == root->left);

			while(node != root){
				next = rb_next(node);
				printf("key : %d ppa : %d cnt : %d\n",node->key, node->h_ppa, node->cnt);
				node = next;
			}
#ifdef RB_LINK
			root->next = root->prev =
#endif
				root->left = root->right = root;


		}
	}


}
int32_t cache_single_entries(C_TABLE *c_table){
	
	Redblack root;
	Redblack node, next;

	root = c_table->rb_tree;
	node = rb_first(root);
	assert(root == root->left);

	while(node != root){
		next = rb_next(node);
		printf("key : %d ppa : %d cnt : %d\n",node->key, node->h_ppa, node->cnt);
		node = next;
	}


}

/*
int32_t cache_mapped_size()
{
        //int32_t idx = max_cache_entry;
        int32_t cache_size = 0;
        int32_t cnt = 0;
	int32_t entry_cnt = 0;
        //int32_t miss_cnt = 0;
	int32_t check_cnt = 0;
        C_TABLE *c_table;
	NODE *lru_node;
	NODE *entry_node;
	
	lru_node = lru->head;
	struct entry_node *ent = NULL;
	
	for(int i = 0 ; i < idx; i++){
		c_table = &CMT[i];
		entry_node = c_table->entry_lru->head;
		while(entry_node != NULL){
			entry_cnt++;
			entry_node = entry_node->next;
		}
	}
	
		
	while(lru_node != NULL){
		c_table = (C_TABLE *)lru_node->DATA;
		
		for(int i = 0 ; i < EPP; i++){
			printf("[%d] = %d\n",i,c_table->p_table[i].ppa);
		}
		
		entry_node = c_table->entry_lru->head;
		while(entry_node != NULL){
			ent = (struct entry_node *)entry_node->DATA;
	//		printf("[%d | %d | %d]\n",ent->p_index, ent->ppa, ent->cnt);

			entry_cnt++;
			entry_node = entry_node->next;
		}
		
		for(int i = 0 ; i < EPP ;i++){
			//printf("[%d] = %d\n",i,c_table->p_table[i].ppa);
			printf("p_table[%d].bitmap = %d\n",i,c_table->h_bitmap[i]);
		}
		break;
		
		//printf("[%d]->flying_mapping_size : %d\n",c_table->idx, c_table->flying_mapping_size);
		check_cnt += c_table->entry_cnt;
		cnt++;
		lru_node = lru_node->next;
	}
	

	

	printf("entry_cnt : %d\n",entry_cnt);
	cache_size = entry_cnt * ENTRY_SIZE;
        printf("Num_translation_page_caching : %d\n",cnt);
        return cache_size;


}
*/
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



