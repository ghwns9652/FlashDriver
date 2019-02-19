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





