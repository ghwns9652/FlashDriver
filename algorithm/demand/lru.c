
/*
 * LRU module implementation
 */

#include "lru.h"
#include <stdlib.h>

int lru_init(struct lru_list **list_ptr) {
	struct lru_list *list = (struct lru_list *)malloc(sizeof(struct lru_list));
	if (!list) {
		return 1;
	}

	list->size = 0;
	list->head = NULL;
	list->tail = NULL;

	*list_ptr = list;

	return 0;
}

int lru_free(struct lru_list *list) {
	while (lru_pop(list)) { /* Until empty */ }
	free(list);

	return 0;
}

struct lru_node *lru_push(struct lru_list *list, void *data) {
	struct lru_node *node = (struct lru_node *)malloc(sizeof(struct lru_node));
	if (!node) {
		return NULL;
	}

	node->data = data;
	node->next = NULL;
	node->prev = NULL;

	if (!list->head) { // Case of initial push
		list->head = node;
		list->tail = node;
	} else { // Common case
		list->head->prev = node;
		node->next = list->head;
		list->head = node;
	}
	++list->size;

	return node;
}

void *lru_pop(struct lru_list *list) {
	void *data;
	struct lru_node *victim = list->tail;

	if (!victim) { // Case of empty
		return NULL;
	}

	if (!victim->prev) { // Case of alone
		list->head = NULL;
		list->tail = NULL;
	} else { // Common case
		list->tail = victim->prev;
		list->tail->next = NULL;
	}

	data = victim->data;
	free(victim);

	--list->size;

	return data;
}

int lru_update(struct lru_list *list, struct lru_node *node) {

	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;

	if (list->head) list->head->prev = node;
	node->next = list->head;
	node->prev = NULL;
	list->head = node;

	return 0;
}

