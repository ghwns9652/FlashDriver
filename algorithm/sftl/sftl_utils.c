#include "dftl.h"


int32_t head_init(C_TABLE *c_table, int32_t ppa)
{
        struct head_node *now;
	if(c_table->head == NULL)
	{
		now = (struct head_node *)malloc(sizeof(struct head_node));
		now->head_ppa = ppa;
		now->next = NULL;
		c_table->head = c_table->tail = now;
		return 1;
        }

	return 0;
        
}

struct head_node* head_push(struct head_node *find_node, int32_t ppa)
{
	struct head_node *now;
	now = (struct head_node *)malloc(sizeof(struct head_node));
	now->head_ppa = ppa;
	now->next = find_node->next;
	find_node->next = now;

	return now;

}

int32_t head_tail_push(C_TABLE *c_table, int32_t ppa)
{
	struct head_node *now;
	
	now = (struct head_node *)malloc(sizeof(struct head_node));
	now->head_ppa = ppa;
	now->next = NULL;
	

	if(c_table->head == NULL){
		c_table->head = c_table->tail = now;
		return 0;	
	}
	else{
		c_table->tail->next = now;
		c_table->tail = now;
	}

	return 1;

}

int32_t head_free(C_TABLE *evic_ptr)
{
        struct head_node *p_node = NULL;
        if(evic_ptr->head == NULL)
                return 0;

        while(evic_ptr->head != NULL)
        {
                p_node = evic_ptr->head;
                evic_ptr->head = evic_ptr->head->next;
                free(p_node);
        }
	evic_ptr->tail = evic_ptr->head;

        return 1;

}
int32_t head_bit_set(int32_t lpa)
{
	struct head_node *now;
	struct head_node *tmp;
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;
	
	int32_t head_ppa = p_table[0].ppa;
	int32_t idx = 1;	
	int32_t b_form_size = 0;
	int32_t cnt = 0;
	c_table->bitmap[0] = 1;
	b_form_size += ENTRY_SIZE;
	for(int i = 1 ; i < EPP; i++){
		if(p_table[i].ppa == head_ppa + idx++){
			c_table->bitmap[i] = 0;
		}else{
			head_ppa = p_table[i].ppa;
			c_table->bitmap[i] = 1;
			idx = 1;
			cnt++;
			b_form_size += ENTRY_SIZE;
		}
	}
	c_table->bit_cnt = cnt;
	b_form_size += BITMAP_SIZE;
	return b_form_size;


	
}
int32_t head_list_set(int32_t lpa){
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;
	int32_t head_ppa;
	for(int i = 0 ; i < EPP; i++){
		if(c_table->bitmap[i] == 1){	
			head_ppa = p_table[i].ppa;
			head_tail_push(c_table, head_ppa);	
		}
	}

}
struct head_node* sftl_list_find(C_TABLE *c_table, int32_t offset)
{
	struct head_node *now = c_table->head;
	for(int i = offset; i > 0 ; i--)
	{
		if(c_table->bitmap[i] == 1)
			now = now->next;
	}

	return now;
}

int32_t sftl_bitmap_set(int32_t lpa)
{
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;

	struct head_node *tmp;
	int32_t offset = P_IDX;
	int32_t head_ppa = p_table[offset].ppa;
	int32_t next_ppa;
	int32_t pre_ppa;
	int32_t idx = 1;

	//If mapping table access is first, Only update head_entry
	if(c_table->first_check){
		c_table->first_check = 0;
		c_table->head->head_ppa = head_ppa;
		return 0;
	}
	tmp = sftl_list_find(c_table, offset);
	if(c_table->bitmap[offset] == 1){
		if(offset == EPP-1){
			tmp->head_ppa = head_ppa;
			return 1;
		}
		tmp->head_ppa = head_ppa;
		next_ppa = p_table[offset+idx].ppa;
		if(next_ppa == -1) return -1;

		if(next_ppa != head_ppa + idx){
			c_table->bitmap[offset+idx] = 1;
			c_table->bit_cnt++;
			c_table->b_form_size += ENTRY_SIZE;
			head_push(tmp,next_ppa);
			
		}

	}
	//bitmap[offset] == 0
	//First offset never enter here
	else{

		pre_ppa = p_table[offset-idx].ppa;
		if(offset == EPP-1){
			if(head_ppa != pre_ppa + idx){
				c_table->bitmap[offset] = 1;
				c_table->bit_cnt++;
				c_table->b_form_size += ENTRY_SIZE;
				head_push(tmp, head_ppa);

			}
		}else{


			next_ppa = p_table[offset+idx].ppa;
			if(head_ppa != pre_ppa + idx){
				c_table->bitmap[offset] = 1;
				c_table->bit_cnt++;
				c_table->b_form_size += ENTRY_SIZE;
				tmp = head_push(tmp, head_ppa);
			}
			if(next_ppa == -1) return -1;
			
			if(next_ppa != head_ppa + idx){
				c_table->bitmap[offset+idx] = 1;
				c_table->bit_cnt++;
				c_table->b_form_size += ENTRY_SIZE;
				head_push(tmp, next_ppa);
			}
			
		}
	}

	return head_ppa;
}

int32_t sftl_bitmap_free(C_TABLE *evic_ptr)
{
	C_TABLE *c_table = evic_ptr;
	head_free(c_table);	
	//memset(c_table->bitmap,0, sizeof(bool) * EPP);
	//c_table->form_check = 0;
	//c_table->bit_cnt = 0;
	return 1;
	
}
int32_t sftl_update_set(int32_t lpa, int32_t size, queue *p_buffer){
	
	//** If cache hit and bitmap form, Set bitmap and head entries
	//*  Support variable request size
	C_TABLE *c_table = NULL;
	D_TABLE *mapping_table = NULL;
	struct head_node *tmp = NULL;

	int32_t start_offset;
	int32_t length;
	int32_t idx;
	int32_t *ppa;
	int32_t head_entry;

	
	c_table      = &CMT[D_IDX];
	start_offset = P_IDX;
	length       = size / PAGESIZE;
//	*ppa         = (int32_t *)q_dequeue(p_buffer);
	head_entry   = *ppa;
	idx          = 1;

	//INDEX is first_offset
	if(start_offset == 0){
		//start_offset is not bitmap 0
	}
	//INDEX is last_offset
	if(start_offset == EPP-1){
	}

		

}
int32_t sftl_bitmap_size(int32_t lpa)
{
	C_TABLE *c_table = &CMT[D_IDX];
	int32_t bitmap_form_size = 0;
	for(int i = 0 ; i < EPP; i++)
	{
		if(c_table->bitmap[i])
		{
			bitmap_form_size += ENTRY_SIZE;
		}
	}
	bitmap_form_size += BITMAP_SIZE;
	return bitmap_form_size;
}
int32_t get_mapped_ppa(int32_t lpa)
{
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;
	struct head_node *now = c_table->head;
	int32_t offset = P_IDX;
	int32_t head_lpn = -1;
	int32_t head_ppn = -1;
	int32_t idx;
	int32_t ppa;
	if(c_table->bit_cnt > 1){
		for(int i = offset ; i > 0 ; i--){
			if(c_table->bitmap[i] == 1){
				now = now->next;
				if(head_lpn == -1)
					head_lpn = i;
			}
		}

		idx = abs(head_lpn - offset);
		head_ppn = now->head_ppa;
		ppa = head_ppn + idx;
	}else{
		head_ppn = now->head_ppa;
		ppa = head_ppn + offset;
	//	printf("offset = %d head_ppn = %d ppa = %d real_ppa = %d\n",offset,head_ppn,ppa,p_table[offset].ppa);
	}
	return ppa;

}

int32_t cache_mapped_size()
{
	int32_t idx = max_cache_entry;
	int32_t cache_size = 0;
	int32_t cnt = 0;
	C_TABLE *c_table = NULL;
	D_TABLE *p_table = NULL;
	for(int i = 0; i < idx; i++)
	{
		c_table = &CMT[i];
		p_table = c_table->p_table;
		if(p_table)
		{
	//		printf("c_table[%d].b_form_size = %d\n",i,c_table->b_form_size);
			cnt++;
			cache_size += c_table->b_form_size;
		}
	}
	
	C_TABLE *tmp = &CMT[817];
	printf("num_caching : %d\n",cnt);
	return cache_size;


}
