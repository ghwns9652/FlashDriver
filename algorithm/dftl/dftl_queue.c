#include "dftl_queue.h"

void initqueue(m_queue **q){
	*q=(m_queue*)malloc(sizeof(m_queue));
	(*q)->size=0;
	(*q)->head=(*q)->tail=NULL;
}

void m_enqueue(m_queue* q, void* data){
	m_node *new_node=(m_node*)malloc(sizeof(m_node));
	new_node->data = data;
	new_node->next=NULL;
	if(q->size==0){
		q->head=q->tail=new_node;
	}
	else{
		q->tail->next=new_node;
		q->tail=new_node;
	}
	q->size++;
}

void* m_dequeue(m_queue *q){
	if(!q->head || q->size==0){
		return NULL;
	}
	m_node *target_node;
	target_node=q->head;
	q->head=q->head->next;

	void *res=target_node->data;
	q->size--;
	free(target_node);
	return res;
}

void freequeue(m_queue* q){
	while(m_dequeue(q)){}
	free(q);
}
