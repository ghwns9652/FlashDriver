#include "dftl.h"


void head_push(struct head_node **head, int32_t ppa)
{
        struct head_node *now;
        now = (struct head_node *)malloc(sizeof(struct head_node));

        now->head_ppa = ppa;
        now->next = NULL;

        if((*head) == NULL)
        {
                *head = now;
        }
        else
        {
                struct head_node *Tail = *head;
                while(Tail->next != NULL)
                {
                        Tail = Tail->next;
                }
                Tail->next = now;
        }
        return ;
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

int32_t bitmap_set(int32_t lpa)
{
	C_TABLE *c_table = &CMT[D_IDX];
	D_TABLE *p_table = c_table->p_table;
	
	//Push first_ppa in p_table
	int32_t head_ppa = p_table[0].ppa;
	int32_t idx = 1;
	//For bitmap_form_size
	int32_t bitmap_form_size = 0;

	head_push(&c_table->head,head_ppa);
	c_table->bitmap[0] = 1;
	bitmap_form_size += ENTRY_SIZE;

	for(int i = 1 ; i < EPP; i++)
	{
		//If ppa is sequential, Set the bitmap = 0;
		if(p_table[i].ppa == head_ppa + idx++)
		{
			c_table->bitmap[i] = 0;
		}
		else
		{
			head_ppa = p_table[i].ppa;
			head_push(&c_table->head,head_ppa);
			c_table->bitmap[i] = 1;
			idx = 1;
			bitmap_form_size += ENTRY_SIZE;
		}	
	}
	bitmap_form_size += BITMAP_SIZE;
	//Bitmap_form_size(Byte) return 
	return bitmap_form_size;
}

int32_t bitmap_free(int32_t lpa)
{
	C_TABLE *c_table = &CMT[D_IDX];
	
	if(head_free(&c_table->head))
	{
		printf("Success bitmap initialization!\n");
		memset(c_table->bitmap,0,sizeof(bool)*EPP); //Bitmap free
		c_table->b_form_size = 0;
	}
	else
	{
		printf("Fail bitmap initialization!\n");
		return 0;
	}
	return 1;
}

