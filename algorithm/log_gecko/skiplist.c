#include "skiplist.h"
skiplist *skiplist_init()
{
	skiplist *point = (skiplist *)malloc(sizeof(skiplist));
	point->level = 1;
	point->header = (snode *)malloc(sizeof(snode));
	point->header->list = (snode **)malloc(sizeof(snode *) * (MAX_L + 1));
	for(int i = 0; i <= MAX_L; i++)
		point->header->list[i] = point->header;
	point->header->key = INT_MAX;
	point->start = 0;
	point->end = UINT_MAX;
	point->size = 0;
	return point;
}

snode *skiplist_find(skiplist *list, KEYT key)
{
	if(list->size == 0)
		return NULL;
	snode *x = list->header;
	for(int i = list->level; i >= 1; i--)
		while(x->list[i]->key < key)
			x = x->list[i];
	if(x->list[1]->key == key)
		return x->list[1];
	return NULL;
}

static int getLevel()
{
	int level = 1;
	int temp = rand();
	while(temp % PROB == 1)
	{
		temp = rand();
		level++;
		if(level + 1 >= MAX_L)
			break;
	}
	return level;
}

snode *skiplist_insert(skiplist *list, KEYT key, uint8_t offset, ERASET flag)
{
	snode *update[MAX_L + 1];
	snode *x = list->header;
	for(int i = list->level; i >= 1; i--)
	{
		while(x->list[i]->key < key)
			x = x->list[i];
		update[i] = x;
	}
	x = x->list[1];

	if(key > list->start)
		list->start = key;
	if(key < list->end)
		list->end = key;

	if(key == x->key)
	{
		if(flag == 1)
		{
			for (int i = 0; i < 4; i++)
				x->VBM[i] = 0;
			x->erase = flag;
		}
		else
			x->VBM[offset / 64] |= ((uint64_t)1 << (offset % 64));
	}
	else
	{
		int level = getLevel();
		if(level > list->level)
		{
			for(int i = list->level + 1; i <= level; i++)
				update[i] = list->header;
			list->level = level;
		}

		x = (snode *)malloc(sizeof(snode));
		x->list = (snode **)malloc(sizeof(snode *) * (level + 1));
		x->key = key;
		if(flag == 1)
		{
			for(int i = 0; i < 4; i++)
				x->VBM[i] = 0;
			x->erase = flag;
		}
		else
		{
			for(int i = 0; i < 4; i++)
				x->VBM[i] = 0;
			x->erase = 0;
			x->VBM[offset / 64] |= ((uint64_t)1 << (offset % 64));
		}
		for(int i = 1; i <= level; i++)
		{
			x->list[i] = update[i]->list[i];
			update[i]->list[i] = x;
		}
		x->level = level;
		list->size++;
	}
	return x;
}
//must insert newest node first
snode *skiplist_merge_insert(skiplist *list, snode *input)
{
	snode *update[MAX_L + 1];
	snode *x = list->header;
	for(int i = list->level; i >= 1; i--)
	{
		while(x->list[i]->key < input->key)
			x = x->list[i];
		update[i] = x;
	}
	x = x->list[1];
	if(input->key == x->key)
	{
		if(x->erase != 1)
		{
			for (int i = 0; i < 4; i++)
				x->VBM[i] |= input->VBM[i];
			x->erase = input->erase;
		}
	}
	else
	{
		int level = getLevel();
		if(level > list->level)
		{
			for(int i = list->level + 1; i <= level; i++)
				update[i] = list->header;
			list->level = level;
		}

		x = (snode *)malloc(sizeof(snode));
		x->list = (snode **)malloc(sizeof(snode *) * (level + 1));
		x->key = input->key;
		for(int i = 0; i < 4; i++)
			x->VBM[i] = input->VBM[i];
		x->erase = input->erase;
		for(int i = 1; i <= level; i++)
		{
			x->list[i] = update[i]->list[i];
			update[i]->list[i] = x;
		}
		x->level = level;
		list->size++;
	}
	return x;
}
// start end 수정 안함
int skiplist_delete(skiplist* list, KEYT key){
	if(list->size==0)
		return -1;
	snode *update[MAX_L+1];
	snode *x=list->header;
	for(int i=list->level; i>=1; i--){
		while(x->list[i]->key<key)
			x=x->list[i];
		update[i]=x;
	}
	x=x->list[1];

	if(x->key!=key)
		return -2; 

	for(int i=x->level; i>=1; i--){
		update[i]->list[i]=x->list[i];
		if(update[i]==update[i]->list[i])
			list->level--;
	}

	free(x->list);
	free(x);
	list->size--;
	return 0;
}

sk_iter *skiplist_get_iterator(skiplist *list)
{
	sk_iter *res = (sk_iter *)malloc(sizeof(sk_iter));
	res->list = list;
	res->now = list->header;
	return res;
}

snode *skiplist_get_next(sk_iter *iter)
{
	if(iter->now->list[1] == iter->list->header) //end
	{
		return NULL;
	}
	else
	{
		iter->now = iter->now->list[1];
		return iter->now;
	}
}

void skiplist_clear(skiplist *list)
{
	snode *now = list->header->list[1];
	snode *next = now->list[1];
	while(now != list->header)
	{
		free(now->list);
		free(now);
		now = next;
		next = now->list[1];
	}
	list->size = 0;
	list->level = 0;
	for(int i = 0; i <= MAX_L; i++)
		list->header->list[i] = list->header;
	list->header->key = INT_MAX;
}

void skiplist_free(skiplist *list)
{
	skiplist_clear(list);
	free(list->header->list);
	free(list->header);
	free(list);
	return;
}

struct node* skiplist_flush(skiplist *list)
{
	struct node* temp = (struct node *)malloc(sizeof(struct node));
	temp->memptr = skiplist_make_data(list);
	temp->max = list->start;
	temp->min = list->end;
	skiplist_free(list);
	return temp;
}

PTR skiplist_make_data(skiplist *input)
{
	PTR Wrappage = (PTR)malloc(PAGESIZE);
	memset(Wrappage, 0, PAGESIZE); //나중엔 노쓸모
	int loc = 0;
	sk_iter *iter = skiplist_get_iterator(input);
	snode *now;
	while(now = skiplist_get_next(iter))
	{
		memcpy(&Wrappage[loc], now, sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET));
		loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
	}
	free(iter);
	return Wrappage;
}
// for test
void skiplist_dump_key(skiplist *list)
{
	sk_iter *iter = skiplist_get_iterator(list);
	snode *now;
	while(now = skiplist_get_next(iter))
	{
		for(int i = 1; i <= now->level; i++)
			printf("%u ", now->key);
		printf("\n");
	}
	free(iter);
}
// for test
void skiplist_dump_key_value(skiplist *list)
{
	sk_iter *iter = skiplist_get_iterator(list);
	snode *now;
	while(now = skiplist_get_next(iter))
	{
		printf("key(%u): hexvalue1(0x%" PRIx64 "), hexvalue2(0x%" PRIx64 "), \
hexvalue3(0x%" PRIx64 "), hexvalue4(0x%" PRIx64 "), erase(%d)\n",
			   now->key, now->VBM[0], now->VBM[1], now->VBM[2], now->VBM[3], now->erase);
	}
	free(iter);
}
