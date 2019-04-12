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
			return find_node;
		
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
			tp_entry_split(find_node, lpa, ppa, 0);
		}
		lru_update(c_table->entry_lru, c_table->last_ptr);

	}
	//CASE 2 : Split
	else{
		index = ent_node->p_index;
		if(index <= P_IDX && P_IDX <= index + ent_node->cnt){
			last_ptr = tp_entry_split(last_ptr, lpa, ppa, 1);
		}else{
			find_node = tp_entry_search(lpa);
			if(find_node != NULL){
				last_ptr = tp_entry_split(find_node,lpa, ppa, 1);
			}else{
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
	//	printf("3\n");
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

	//	printf("1\n");
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
		
	//	printf("2\n");
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

int32_t tp_get_ppa(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *entry_lru = c_table->entry_lru->head;
	int32_t ppa = -1;
	int32_t head_ppn;
	int32_t head_lpn;
	int32_t offset;
	while(entry_lru != NULL){

		struct entry_node *ent_node = (struct entry_node*)entry_lru->DATA;
		if(ent_node->p_index <= P_IDX && P_IDX <= ent_node->p_index + ent_node->cnt){
			head_lpn = ent_node->p_index;
			head_ppn = ent_node->ppa;
			offset = P_IDX - head_lpn;
			ppa = head_ppn + offset;

			//printf("%d = [%d | %d | %d]\n",P_IDX, head_lpn, head_ppn, ent_node->cnt);
			return ppa;

		}

		entry_lru = entry_lru->next;
	}
	return ppa;
}


int32_t cache_mapped_size()
{
        int32_t idx = max_cache_entry;
        int32_t cache_size = 0;
        int32_t cnt = 0;
        int32_t miss_cnt = 0;
        C_TABLE *c_table;
        D_TABLE *p_table;
	LRU *entry_lru = NULL;
	NODE *entry_node = NULL;
	struct entry_node *ent = NULL;
	for(int i = 0; i<idx; i++){
		c_table = &CMT[i];
		entry_lru = c_table->entry_lru;
		entry_node = entry_lru->head;
		if(i == 0){
			if(entry_node != NULL){
				while(entry_node != NULL){
					ent = (struct entry_node *)entry_node->DATA;
					printf("%d = [%d | %d | %d]\n",i, ent->p_index, ent->ppa, ent->cnt);
					entry_node = entry_node->next;
				}
				cnt++;
			}
			else{
				miss_cnt++;
			}
			break;
		}
	}
	cache_size = cnt * ENTRY_SIZE;
        printf("not_caching : %d\n",miss_cnt);
        printf("num_caching : %d\n",cnt);
        return cache_size;


}

