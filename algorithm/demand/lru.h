
/*
 * LRU module Header File
 */

#ifndef __DEMAND_LRU_H__
#define __DEMAND_LRU_H__

struct lru_node {
	void *data;
	struct lru_node *next;
	struct lru_node *prev;
};

struct lru_list {
	int size;
	struct lru_node *head;
	struct lru_node *tail;
};

int lru_init(struct lru_list **);
int lru_free(struct lru_list *);
struct lru_node *lru_push(struct lru_list *, void *);
void *lru_pop(struct lru_list *);
int lru_update(struct lru_list *, struct lru_node *);

#endif
