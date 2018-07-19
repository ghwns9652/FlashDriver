#ifndef __DFTL_Q_H__
#define __DFTL_Q_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct m_node{
	void *data;
	struct m_node *next;
}m_node;

typedef struct m_queue{
	int size;
	m_node *head;
	m_node *tail;
}m_queue;

void initqueue(m_queue **q);
void freequeue(m_queue *q);
void m_enqueue(m_queue *q, void* node);
void* m_dequeue(m_queue *q);

#endif
