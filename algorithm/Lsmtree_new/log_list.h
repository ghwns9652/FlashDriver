#ifndef __LIST_H__
#define __LIST_H__
#include "../../include/settings.h"

#define for_each_log_block(temp,b)\
	for(temp=(b)->head;temp;temp=temp->next)

typedef struct llog_node{
	void *data;
	struct llog_node *next;
	struct llog_node *prev;
}llog_node;

typedef struct{
	llog_node *tail;
	llog_node *head;
	llog_node *last_valid;//for block;
	int size;
}llog;

llog* llog_init();
llog_node* llog_insert(llog *,void *);
llog_node* llog_insert_back(llog *, void *);
llog_node* llog_insert_check(llog*, void*);
llog_node* llog_next(llog_node *);

void llog_checker(llog *);
void llog_delete(llog *,llog_node *);
void llog_free(llog *log);
void llog_move_back(llog *body, llog_node *target);
void llog_move_blv(llog *body,llog_node* target);
int llog_erase_cnt(llog *body);
void llog_print(llog *body);
bool llog_isthere(llog*, uint32_t pba);

#endif
