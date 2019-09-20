#include "log_list.h"
#include "page.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CASTBL(data) ((block*)(data))
#define BLDATATBN(data) (((block*)(data))->ppa/_PPB)
llog* llog_init(){
	llog *res=(llog*)malloc(sizeof(llog));
	res->head=NULL;
	res->tail=NULL;
	res->size=0;
	return res;
}

extern pm data_m;
llog_node* llog_insert(llog *l,void *data){
	llog_node *new_l=(llog_node*)malloc(sizeof(llog_node));
	new_l->data=data;
	new_l->prev=NULL;
	new_l->next=NULL;

	if(!l->head && !l->tail){
		l->head=l->tail=new_l;
	}
	else{
		l->head->prev=new_l;
		new_l->next=l->head;
		l->head=new_l;
		l->last_valid=new_l;
	}
	l->size++;
	return new_l;
}

llog_node* llog_insert_back(llog *l,void *data){
	llog_node *new_l=(llog_node*)malloc(sizeof(llog_node));
	new_l->data=data;
	new_l->prev=NULL;
	new_l->next=NULL;


	if(!l->head && !l->tail){
		l->head=l->tail=new_l;
	}
	else{
		new_l->prev=l->tail;
		l->tail->next=new_l;
		l->tail=new_l;
	}
	l->size++;
	return new_l;
}

void llog_delete(llog *body, llog_node *target){
	if(BLDATATBN(target->data)==192){
	//	printf("break!\n");
	}
	llog_node *nxt=target->next;
	llog_node *prv=target->prev;

	if(prv){
		prv->next=nxt;
	}
	if(nxt){
		nxt->prev=prv;
	}
	
	if(target==body->tail){
		body->tail=prv;
	}
	if(target==body->head){
		body->head=nxt;
	}

	free(target);
	body->size--;
}

void llog_move_back(llog *body, llog_node *target){
	if(body->tail==target){
		return;
	}

	llog_node *nxt=target->next;
	llog_node *prv=target->prev;

	if(prv){
		prv->next=nxt;
	}

	if(nxt){
		nxt->prev=prv;
	}
	
	if(body->head==target){
		body->head=target->next;
	}

	target->next=body->tail->next;
	body->tail->next=target;
	target->prev=body->tail;
	body->tail=target;	
}

void llog_move_blv(llog *body, llog_node *target){
	llog_node *nxt=target->next;
	llog_node *prv=target->prev;
	if(prv)
		prv->next=nxt;
	if(nxt)
		nxt->prev=prv;

	if(body->head==target){
		body->head=nxt;
	}

	llog_node *lv=body->last_valid;
	nxt=lv->next;
	//lv or->next
	if(nxt)
		nxt->prev=target;

	target->next=nxt;
	target->prev=lv;
	lv->next=target;

	if(body->tail==lv){
		body->tail=target;
	}

	body->last_valid=target;
}

llog_node *llog_next(llog_node *in){
	if(!in) return NULL;
	llog_node *res=in->prev;
	return res?res:NULL;
}

void llog_print(llog *b){
	fprintf(stderr,"size: %d\n",b->size);
	llog_node *head=b->head;
	int cnt=0;
	int ecnt=0;
	while(head){
		block* target=(block*)head->data;
		if(target->erased)
			ecnt++;
		fprintf(stderr,"[%d]ppa:%d(%d) ppage_idx:%d invalid_n:%d level:%d erased:%d bitset:%p\n",target->ppa/_PPB,target->ppa,target->ppa/BPS/_PPB,target->ppage_idx,target->invalid_n,target->level,target->erased?1:0,target->bitset);
		head=head->next;
		cnt++;
		if(cnt>b->size) break;
	}
	fprintf(stderr,"cnt:%d ecnt:%d\n",cnt,ecnt);
}

int llog_erase_cnt(llog *b){
	//printf("size: %d\n",b->size);
	llog_node *head=b->head;
	int cnt=0;
	int ecnt=0;
	while(head){
		block* target=(block*)head->data;
		if(target->erased){
	//		printf("%d\n",target->ppa/256);
			ecnt++;
		}
		else break;
		head=head->next;
		cnt++;
		if(cnt>b->size) break;
	}
	return ecnt;
}

bool llog_isthere(llog *b,uint32_t pba){
	//printf("size: %d\n",b->size);
	llog_node *head=b->head;
	while(head){
		block* target=(block*)head->data;
		if(target->ppa/256==pba)
			return true;
		head=head->next;
	}
	return false;
}

void llog_free(llog *l){
	llog_node *temp=l->tail;
	llog_node *prev;
	while(temp){
		prev=llog_next(temp);
		free(temp);
		temp=prev;
	}
	free(l);
}

void llog_checker(llog *l){
	llog_node *t;
	bool before_zero=false, start_flag=false;
	for_each_log_block(t,l){
		if(start_flag){
			if(before_zero && CASTBL(t->data)->erased){
				llog_print(l);
				abort();
			}
		}else{
			start_flag=true;
			if(CASTBL(t->data)->erased) before_zero=false;
			else before_zero=true;
		}
	}
}
