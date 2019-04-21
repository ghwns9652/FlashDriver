#include "dftl.h"

NODE *tp_entry_search(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *find_node = c_table->entry_lru->head;
	struct entry_node *ent_node = (struct entry_node *)find_node->DATA;
	int32_t offset = P_IDX;
	while(find_node != NULL){
		if(find_node == c_table->last_ptr){
			find_node = find_node->next;
			continue;
		}

		ent_node = (struct entry_node *)find_node->DATA;
		if(ent_node->p_index <= offset && offset <= ent_node->p_index + ent_node->cnt)
		{
			return find_node;
		}		
		find_node = find_node->next;
	}
	return NULL;
}

struct entry_node *tp_entry_alloc(int32_t offset, int32_t ppa, int32_t cnt, char req_t){	
	struct entry_node *now = (struct entry_node *)malloc(sizeof(struct entry_node));
	now->p_index = offset;
	now->ppa = ppa;
	now->cnt = cnt;
	if(req_t == 'W')
		now->state = DIRTY;
	else
		now->state = CLEAN;
	return now;
}
NODE *tp_entry_update(NODE *update_node, int32_t offset, int32_t ppa, int32_t cnt, char req_t){
	struct entry_node *update_ent = (struct entry_node *)update_node->DATA;
	update_ent->p_index = offset;
	update_ent->ppa = ppa;
	update_ent->cnt = cnt;
	if(req_t == 'W')
		update_ent->state = DIRTY;
	else
		update_ent->state = CLEAN;

	return update_node;
}

NODE *tp_entry_op(int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *last_ptr = c_table->last_ptr;
	NODE *find_node = NULL;
	struct entry_node *ent_node = (struct entry_node *)last_ptr->DATA;
	struct entry_node *now = NULL;
	int32_t offset = P_IDX;
	int32_t index;	
	int32_t pre_ppa;
	
	//CASE 1 : Merge
	
	index   = ent_node->p_index + ent_node->cnt + 1;           //Variable For check current offset
	pre_ppa = ent_node->ppa + (P_IDX - ent_node->p_index - 1); //Get a Pre ppn
	if(index == P_IDX && ppa == pre_ppa+1){

		if(ent_node->cnt == MAX_CNT){
			now = tp_entry_alloc(offset, ppa, 0, 'W');
			last_ptr = lru_push(c_table->entry_lru, (void *)now);
			c_table->entry_cnt++;
			free_cache_size -= ENTRY_SIZE;
			return last_ptr;
		}

		ent_node->cnt++;
		find_node = tp_entry_search(lpa);
	        	
		if(find_node != NULL){
//			printf("P_IDX = %d\n",P_IDX);
			tp_entry_split(find_node, lpa, ppa, 0);
		}
		lru_update(c_table->entry_lru, c_table->last_ptr);

	}
	//CASE 2 : Split
	else{
		index = ent_node->p_index;
		
		//printf("P_IDX = %d ppa = %d\n",P_IDX,ppa);
		//printf("index= %d cnt = %d\n",index,ent_node->cnt);
		
		if(index <= P_IDX && P_IDX <= index + ent_node->cnt){
			last_ptr = tp_entry_split(last_ptr, lpa, ppa, 1);
		}
		else{

			find_node = tp_entry_search(lpa);
			if(find_node != NULL){
				last_ptr = tp_entry_split(find_node,lpa, ppa, 1);
			}else{

				
		//		printf("2_lpa = %d, ppa = %d\n",lpa,ppa);
				now = tp_entry_alloc(offset, ppa, 0,'W');
				last_ptr = lru_push(c_table->entry_lru, (void *)now);
				c_table->entry_cnt++;
				free_cache_size -= ENTRY_SIZE;
			}
				
		}

	}

	return last_ptr;
	
}
NODE *tp_entry_split(NODE *split_node, int32_t lpa, int32_t ppa, bool flag){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *last_ptr = NULL;
	struct entry_node *ent_node = (struct entry_node *)split_node->DATA;
	struct entry_node *now = NULL;
	int32_t head_lpn = ent_node->p_index;
	int32_t head_ppn = ent_node->ppa;
	int32_t cnt = ent_node->cnt;
	int32_t distance = 0;
	int32_t pre_cnt;
	int32_t next_cnt;
	if(P_IDX == head_lpn){
		//Split After merge
		if(!flag){
			if(cnt == 0){
				lru_delete(c_table->entry_lru, split_node);
				free_cache_size += ENTRY_SIZE;
				c_table->entry_cnt--;
				return c_table->last_ptr;
			}
			tp_entry_update(split_node, head_lpn+1, head_ppn+1, ent_node->cnt-1,'W');
			lru_update(c_table->entry_lru, split_node);
		//Just split	
		}else{
			if(cnt == 0){
				last_ptr = tp_entry_update(split_node, P_IDX, ppa, ent_node->cnt, 'W');
				lru_update(c_table->entry_lru, last_ptr);
				return last_ptr;
			}
			tp_entry_update(split_node, head_lpn+1, head_ppn+1, ent_node->cnt-1, 'W');
			lru_update(c_table->entry_lru, split_node);
			now = tp_entry_alloc(P_IDX, ppa, 0,'W');
			last_ptr = lru_push(c_table->entry_lru, (void *)now);
			c_table->entry_cnt++;
			free_cache_size -= ENTRY_SIZE;
		}

	}
	else if (head_lpn < P_IDX && P_IDX < head_lpn + cnt){

		distance = P_IDX - head_lpn;
		pre_cnt  = (distance) - 1;
		next_cnt = (cnt - distance) - 1;
		//next_node insert
		tp_entry_update(split_node, P_IDX+1, head_ppn+distance+1, next_cnt,'W');
		lru_update(c_table->entry_lru, split_node);
		//middle_node insert
		now = tp_entry_alloc(P_IDX, ppa, 0,'W');
		last_ptr = lru_push(c_table->entry_lru, (void *)now);
		//first_node insert
		now = tp_entry_alloc(head_lpn, head_ppn, pre_cnt, 'W');
		lru_push(c_table->entry_lru, (void *)now);
		free_cache_size -= ENTRY_SIZE * 2;
		c_table->entry_cnt += 2;

	}
	else{
		
		now = tp_entry_alloc(P_IDX,ppa, 0, 'W');
	//	printf("now->lpn : %d now->ppn: %d\n",now->p_index, now->ppa);
		last_ptr = lru_push(c_table->entry_lru, (void *)now);
		c_table->entry_cnt++;
		free_cache_size -= ENTRY_SIZE;
		if(cnt == 0){
			lru_delete(c_table->entry_lru, split_node);
			free_cache_size += ENTRY_SIZE;
			c_table->entry_cnt--;
			return last_ptr;
		}
	//	printf("ent_node->cnt = %d\n",ent_node->cnt);
		tp_entry_update(split_node, head_lpn, head_ppn, ent_node->cnt-1,'W');
		ent_node = (struct entry_node *)split_node->DATA;	
	//	printf("split_node->lpn : %d ppn : %d cnt = %d\n",ent_node->p_index, ent_node->ppa,ent_node->cnt);
		lru_update(c_table->entry_lru, split_node);
	}

	
	return last_ptr;
}
void tp_batch_update(C_TABLE *evic_ptr){
	NODE *entry_head = evic_ptr->entry_lru->head;
	struct entry_node *update_ent = NULL;

	if(entry_head == NULL){
		return ;
	}
	while(entry_head != NULL){
		update_ent = (struct entry_node *)entry_head->DATA;
		if(update_ent->state == DIRTY){
			update_ent->state = CLEAN;
		}
		entry_head = entry_head->next;
	}
	return ;
}



NODE* tp_get_entry(int32_t lpa, int32_t offset){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *entry_node = c_table->entry_lru->head;
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
			
		//	printf("%d = [%d | %d | %d]\n",P_IDX, head_lpn, head_ppn, ent_node->cnt);
			cnt++;
			find_node = entry_node;
			if(cnt > 1){
		//		printf("중복 노드 발견!\n");
			}

		}

		entry_node = entry_node->next;
	}

	return find_node;
}
NODE *tp_fetch(int32_t lpa, int32_t ppa){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *last_ptr = c_table->last_ptr;
	NODE *pre_find = NULL;
	NODE *next_find = NULL;
	struct entry_node *now;
	struct entry_node *ent_node;
	int32_t pre_ppa, pre_index;
        int32_t cur_ppa;
	int32_t next_ppa;	
	int32_t cnt = 0;
	int32_t merge_cnt = 0;
	int32_t i;
	if(ppa == -1){
		return NULL;
	}
	if(P_IDX == 0){
		next_ppa = c_table->p_table[P_IDX+1].ppa;
		if(next_ppa == -1 || next_ppa != ppa+1){
			now = tp_entry_alloc(P_IDX, ppa, 0, 'R');
			last_ptr = lru_push(c_table->entry_lru, (void *)now);
			c_table->entry_cnt++;
			free_cache_size -= ENTRY_SIZE;
			return last_ptr;
		}
		if(next_ppa == ppa+1){
			next_find = tp_get_entry(lpa, P_IDX+1);
			if(next_find != NULL){
				ent_node = (struct entry_node *)next_find->DATA;
				last_ptr = tp_entry_update(next_find, P_IDX, ppa, ent_node->cnt+1, 'R');
				lru_update(c_table->entry_lru, last_ptr);
			}else{
				for(i = P_IDX ; i < EPP-1; i++){
					cur_ppa  = c_table->p_table[i].ppa;
					next_ppa = c_table->p_table[i+1].ppa;
					if(next_ppa == cur_ppa + 1){
						cnt++;
					}else{
						break;
					}
					if(cnt == MAX_CNT)
						break;
				}
				//P_IDX == 0 , ppa = c_table->p_table[P_IDX].ppa
				now = tp_entry_alloc(P_IDX, ppa, cnt, 'R');
				last_ptr = lru_push(c_table->entry_lru, (void *)now);
				c_table->entry_cnt++;
				free_cache_size -= ENTRY_SIZE;
			}
		}
	}
	//0 < P_IDX < EPP
	else if (P_IDX < EPP){
		pre_ppa = c_table->p_table[P_IDX-1].ppa;
		next_ppa = c_table->p_table[P_IDX+1].ppa;
		if(pre_ppa == -1 || ppa != pre_ppa+1){
			now = tp_entry_alloc(P_IDX, ppa, 0, 'R');
			last_ptr = lru_push(c_table->entry_lru, (void *)now);
			c_table->entry_cnt++;
			free_cache_size -= ENTRY_SIZE;
		}
		else if (ppa == pre_ppa+1){
			pre_find = tp_get_entry(lpa, P_IDX-1);
			//Merge request into pre_node
			if(pre_find != NULL){
				ent_node = (struct entry_node *)pre_find->DATA;
				//Increment count variable
				last_ptr = tp_entry_update(pre_find, ent_node->p_index, ent_node->ppa, ent_node->cnt+1,'R');
				lru_update(c_table->entry_lru, last_ptr);
			}else{
				for(i = P_IDX; i > 0; i--){
					pre_ppa = c_table->p_table[i-1].ppa;
					cur_ppa = c_table->p_table[i].ppa;
					if(cur_ppa == pre_ppa +1){
						cnt++;
					}else{
						break;
					}
				}
				if(i == 0){
					pre_ppa = c_table->p_table[i].ppa;
					if(cur_ppa == pre_ppa +1){
						cur_ppa = pre_ppa;
					}
				}
				now = tp_entry_alloc(i, cur_ppa, cnt, 'R');
				last_ptr = lru_push(c_table->entry_lru, (void *)now);
				c_table->entry_cnt++;
				free_cache_size -= ENTRY_SIZE;
			}
		}

		if(next_ppa == -1 || next_ppa != ppa+1){
			return last_ptr;
		}
		else if(next_ppa == ppa+1){
			next_find = tp_get_entry(lpa, P_IDX+1);
			//CASE 1 : next_find is null
			if(next_find == NULL){
				for(i = P_IDX ; i < EPP-1; i++){
					cur_ppa  = c_table->p_table[i].ppa;
					next_ppa = c_table->p_table[i+1].ppa;
					if(next_ppa == cur_ppa + 1){
						merge_cnt++;
					}else{
						break;
					}
				}
				ent_node = (struct entry_node *)last_ptr->DATA;
				ent_node->cnt += merge_cnt;
				return last_ptr;

			}
			//CASE2 : merge request into within range (e.g, range(1,10) LPN = 5)
			if(last_ptr == next_find){
				last_ptr = next_find;
				return last_ptr;
			}

			//CASE3 : merge pre_node and next_node
			if(next_find != NULL){
				ent_node = (struct entry_node *)next_find->DATA;
				merge_cnt = ent_node->cnt+1;
				ent_node = (struct entry_node *)last_ptr->DATA;
				ent_node->cnt += merge_cnt;

				//next_node delete
				lru_delete(c_table->entry_lru, next_find);
				c_table->entry_cnt--;
				free_cache_size += ENTRY_SIZE;
			}
		}			

	}
	else{
		pre_ppa = c_table->p_table[P_IDX-1].ppa;
		if(pre_ppa == -1 || ppa != pre_ppa+1){
			now = tp_entry_alloc(P_IDX, ppa, 0, 'R');
			last_ptr = lru_push(c_table->entry_lru, (void *)now);
			c_table->entry_cnt++;
			free_cache_size -= ENTRY_SIZE;
			return last_ptr;
		}

		if(ppa == pre_ppa+1){
			pre_find = tp_get_entry(lpa, P_IDX-1);
			if(pre_find != NULL)
			{
				ent_node = (struct entry_node *)pre_find->DATA;
				last_ptr = tp_entry_update(pre_find, ent_node->p_index, ent_node->ppa, ent_node->cnt+1,'R');
				lru_update(c_table->entry_lru, last_ptr);
			}else{
				for(i = P_IDX; i > 0; i--){
					pre_ppa = c_table->p_table[i-1].ppa;
					cur_ppa = c_table->p_table[i].ppa;
					if(cur_ppa == pre_ppa +1){
						cnt++;
					}else{
						break;
					}
				}
				if(i== 0){
					pre_ppa = c_table->p_table[i].ppa;
					if(cur_ppa == pre_ppa + 1){
						cur_ppa = pre_ppa;
					}
				}
				now = tp_entry_alloc(i, cur_ppa, cnt, 'R');
				last_ptr = lru_push(c_table->entry_lru, (void *)now);
				c_table->entry_cnt++;
				free_cache_size -= ENTRY_SIZE;
			}
		}
	}

	return last_ptr;
}
/*
NODE *tp_prefetch(int32_t pf_idx, int32_t pf_ppa){
	//C_TABLE *c_table = &CMT[D_IDX];
	NODE *last_ptr = c_table->last_ptr;
	NODE *pre_ptr = NULL;
	NODE *pre_find = NULL;
	NODE *next_find = NULL;
	struct entry_node *now;
	struct entry_node *ent_node;
	int32_t pre_ppa;
	int32_t next_ppa;
	int32_t p_cnt;
	int32_t entry_cnt = c_table->entry_cnt;
	
	//This request is loaded to pre_node	
}
*/
int32_t cache_mapped_size()
{
        int32_t idx = max_cache_entry;
        int32_t cache_size = 0;
        int32_t cnt = 0;
	int32_t entry_cnt = 0;
        int32_t miss_cnt = 0;
	int32_t check_cnt = 0;
        C_TABLE *c_table;
	NODE *lru_node;
	NODE *entry_node;
	
	lru_node = lru->head;
	struct entry_node *ent = NULL;
	/*
	for(int i = 0 ; i < idx; i++){
		c_table = &CMT[i];
		entry_node = c_table->entry_lru->head;
		while(entry_node != NULL){
			entry_cnt++;
			entry_node = entry_node->next;
		}
	}
	*/
		
	while(lru_node != NULL){
		c_table = (C_TABLE *)lru_node->DATA;
		/*
		for(int i = 0 ; i < EPP; i++){
			printf("[%d] = %d\n",i,c_table->p_table[i].ppa);
		}
		*/
		entry_node = c_table->entry_lru->head;
		while(entry_node != NULL){
			ent = (struct entry_node *)entry_node->DATA;
		//	printf("[%d | %d | %d]\n",ent->p_index, ent->ppa, ent->cnt);

			entry_cnt++;
			entry_node = entry_node->next;
		}

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

