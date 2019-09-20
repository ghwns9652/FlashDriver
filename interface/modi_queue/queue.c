#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include "../include/FS.h"
#include "../include/container.h"
#include "../include/settings.h"

void q_init(queue **q,int qsize){
	*q=(queue*)malloc(sizeof(queue));
	(*q)->size=0;
	(*q)->head=(*q)->tail=NULL;
	//pthread_mutex_init(&((*q)->q_lock),NULL);
	//printf("mutex_t : %p q:%p, size:%d\n",&(*q)->q_lock,*q,qsize);
	//(*q)->firstFlag=true;
	(*q)->m_size=qsize;
    (*q)->head=(node*)malloc(qsize*sizeof(node)); //temp
    memset((*q)->head, 0, qsize*sizeof(node));
    (*q)->enq=0;
    (*q)->deq=0;
}

bool q_enqueue(void* req, queue* q){
    node* arr = q->head;
    int enq = q->enq;

    if(arr[enq].req != NULL){
        return false;
    }

    arr[enq].req = req;
    q->enq++;
    q->enq = (q->enq)%(q->m_size);

	return true;
}

bool q_enqueue_front(void *req, queue*q){
    /*
	pthread_mutex_lock(&q->q_lock);
	if(q->size==q->m_size){	
		pthread_mutex_unlock(&q->q_lock);
		return false;
	}
	node *new_node=(node*)malloc(sizeof(node));
	new_node->req=req;
	new_node->next=NULL;
	if(q->size==0){
		q->head=q->tail=new_node;
	}
	else{
		new_node->next=q->head;
		q->head=new_node;
	}
//	printf("ef-key:%u\n",((request*)req)->key);
	q->size++;
	pthread_mutex_unlock(&q->q_lock);
	return true;
    */
    return true;
}

void* q_dequeue(queue *q){
    node* arr = q->head;
    int deq = q->deq;

    if(arr[deq].req == NULL){
        return NULL;
    }

    void* res = arr[deq].req;
    arr[deq].req = NULL;
    q->deq++;
    q->deq = (q->deq)%(q->m_size);
    return res;
}

void* q_pick(queue *q){
    /*
	pthread_mutex_lock(&q->q_lock);
	if(!q->head || q->size==0){
		pthread_mutex_unlock(&q->q_lock);
		return NULL;
	}
	node *target_node;
	target_node=q->head;
	void *res=target_node->req;
	pthread_mutex_unlock(&q->q_lock);
	return res;
    */
    return NULL;
}

void q_free(queue* q){
	while(q_dequeue(q)){}
	//pthread_mutex_destroy(&q->q_lock);
    free(q->head);
	free(q);
}
