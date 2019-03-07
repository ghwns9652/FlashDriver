#include "dftl.h"


int32_t head_init(struct head_node **head, int32_t ppa)
{
        struct head_node *now;
	if((*head) == NULL)
	{
		now = (struct head_node *)malloc(sizeof(struct head_node));
		now->head_ppa = ppa;
		now->next = NULL;
                *head = now;
		return 1;
        }

	return 0;
        
}

void head_push(struct head_node *find_node, int32_t ppa)
{
	struct head_node *now;
	now = (struct head_node *)malloc(sizeof(struct head_node));
	now->head_ppa = ppa;
	now->next = find_node->next;
	find_node->next = now;

	return ;

}

int32_t head_tail_push(struct head_node **head, int32_t ppa)
{
	struct head_node *now;
	struct head_node *tail;
	
	now = (struct head_node *)malloc(sizeof(struct head_node));
	now->head_ppa = ppa;
	now->next = NULL;


	if(*head == NULL){
		*head = now;	
		return 0;	
	}
	else{
		tail = *head;
		while(tail->next != NULL)
			tail = tail->next;
		tail->next = now;
	}

	return 1;

}

int32_t head_free(struct head_node **head)
{
        struct head_node *p_node = NULL;
        if(*head == NULL)
                return 0;

        while((*head) != NULL)
        {
                p_node = (*head);
                (*head) = (*head)->next;
                free(p_node);
        }

        return 1;

}
int32_t head_find(struct head_node **head, int32_t cnt)
{
        struct head_node *tmp = *head;
        int32_t ppa;
        for(int i = 0 ; i < cnt; i++)
        {
                tmp = tmp->next;
        }

        ppa = tmp->head_ppa;
        return ppa;
}

int32_t head_list_set(int32_t lpa)
{
	struct head_node *now;
	struct head_node *tmp;
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;
	
	//If request is first, return 0;
	

	int32_t head_ppa = p_table[0].ppa;
	c_table->bitmap[0] = 1;
	head_init(&c_table->head, head_ppa);


	for(int i = 1 ; i < EPP; i++){
		if(p_table[i].ppa == (head_ppa + idx++)){
			c_table->bitmap[i] = 0;
		}else{
			head_ppa = p_table[i].ppa;
			c_table->bitmap[i] = 1;
			idx = 1;
		}
	}

	for(int i = 0 ; i < EPP; i++){
		if(c_table->bitmap[i] == 1){
			head_ppa = p_table[i].ppa;
			head_tail_push(&c_table->head, head_ppa);	
		}
	}
	return 1;
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
	int32_t idx = 1;
	//If First head not exists, Make first head entry
	if(!c_table->first_head_check){
		c_table->first_head_check = 1;
		c_table->bitmap[0] = 1;
		head_init(&c_table->head, p_table[0].ppa);
	}
	//If offset is last, push head_ppa in list_tail
	if(offset == EPP-1){
		c_table->bitmap[offset] = 1;
		head_tail_push(&c_table->head, head_ppa);
		return 1;
	}
	
	tmp = sftl_list_find(c_table, offset);
	//Check overwrite
	if(c_table->bitmap[offset] == 1){
		tmp->head_ppa = head_ppa;
		if(c_table->bitmap[offset+idx] == 1)
			return 1;

		if(p_table[offset + idx].ppa != head_ppa + idx)
		{
			c->table->bitmap[offset+idx] = 1;
			head_push(tmp, p_table[offset+idx].ppa);	
		}
	}
	else
	{
		head_push(tmp,head_ppa);
		if(c_table->bitmap[offset+idx] == 1)
			return 1;
		if(p_table[offset+idx].ppa != head_ppa + idx)
		{
			c_table->bitmap[offset+idx] = 1;
			tmp = tmp->next;
			head_push(tmp, p_table[offset+idx].ppa);
		}

	}

	return 0;



}

int32_t sftl_bitmap_free(C_TABLE *evic_ptr)
{
	C_TABLE *c_table = evic_ptr;
	head_free(&c_table->head);
	memset(c_table->bitmap,0,sizeof(bool) * EPP);
	c_table->form_check = 0;
	c_table->b_form_size = 0;
	c_table->first_head_check = 0;
	return 1;
	
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
	int32_t idx = lpa % EPP;
	int32_t cnt = 0;
	int32_t head_lpn = -1;
	int32_t head_ppn = -1;
	int32_t offset = 0;
	int32_t ppa;

	for(int i = P_IDX; i >= 0; i--)
	{
		if(c_table->bitmap[i])
		{
			if(head_lpn == -1)
			{
				head_lpn = i;
			}
			cnt++;
		}
	}

	offset = abs(idx - head_lpn);
	head_ppn = head_find(&c_table->head,cnt-1);
	ppa = head_ppn + offset;
	return ppa;

}

int32_t cache_mapping_size()
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
			cnt++;
			//This is bitmap_form
			if(c_table->form_check)
			{
				cache_size += sftl_bitmap_size(i);
			}
			//This is In-flash form
			else
			{
				cache_size += PAGESIZE;
			}
		}
	}
	printf("num_caching : %d\n",cnt);
	return cache_size;


}
