#include "dftl.h"


NODE* tp_hit_check(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	NODE *entry_node = c_table->entry_lru->head;
	struct entry_node *tmp = NULL;
	int32_t pre_lpn;
	int32_t pre_ppn;
	if(entry_node == NULL)
		return NULL;
	
	while(entry_node != NULL){
		tmp = (struct entry_node *)entry_node->DATA;
		if(tmp->p_index == P_IDX){
			return entry_node;
		}
		if(tmp->p_index < P_IDX){
			for(int i = tmp->p_idex; i <= tmp->p_idex + tmp->cnt; i++){
				if(i == P_IDX){
					return entry_node; 
				}
			}
		}
		entry_node = entry_node->next;
	}
	return NULL;

}

struct entry_node *tp_entry_alloc(int32_t lpa, int32_t ppa, char req_t){	
	C_TABLE *c_table->&CMT[D_IDX];
	int32_t lpn = D_IDX;
	struct entry_node *now = (struct entry_node *)malloc(sizeof(struct entry_node));
	now->p_index = lpn;
	now->ppa = ppa;
	now->cnt = 0;	
	if(req_t == 'W')
		now->state = DIRTY;
	else
		now->state = CLEAN;
	c_table->entry_cnt++;
	return ent;
}

struct entry_node *tp_entry_split(int32_t lpa, int32_t ppa){
}

