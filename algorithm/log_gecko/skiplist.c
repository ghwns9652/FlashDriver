#include "./header/skiplist.h"

skiplist *skiplist_init()
{
	skiplist *point = (skiplist*)malloc(sizeof(skiplist));
	point->level = 1;
	point->header = (snode*)malloc(sizeof(snode));
	point->header->list = (snode**)malloc(sizeof(snode) * (MAX_L + 1));
	for(int i = 0; i < MAX_L; i++)
		point->header->list[i] = point->header;
	point->header->key = INT_MAX;
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
//여기부터 수정
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
	if(key == x->key)
	{
		if(flag == 1)
		{
			for(int i = 0; i < 4; i++)
				x->VBM[i] = 0;
			x->erase = flag;
		}
		else
			x->VBM[offset / VALUE] |= ((uint64_t)1 << (offset % VALUE));
		return x;
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

		x = (snode*)malloc(sizeof(snode));
		x->list = (snode**)malloc(sizeof(snode*) * (level + 1));
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
			x->VBM[offset / VALUE] |= ((uint64_t)1 << (offset % VALUE));	
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

int skiplist_delete(skiplist* list, KEYT key)
{
	if(list->size == 0)
		return -1;
	snode *update[MAX_L + 1];
	snode *x = list->header;
	for(int i = list->level; i >= 1; i--)
	{
		while(x->list[i]->key < key)
			x = x->list[i];
		update[i] = x;
	}
	x = x->list[1];

	if(x->key != key)
		return -2;

	for(int i=x->level; i>=1; i--)
	{
		update[i]->list[i] = x->list[i];
		if(update[i] == update[i]->list[i])
			list->level--;
	}

	free(x->list);
	free(x);
	list->size--;
	return 0;
}

sk_iter* skiplist_get_iterator(skiplist *list)
{
	sk_iter *res = (sk_iter*)malloc(sizeof(sk_iter));
	res->list = list;
	res->now = list->header;
	return res;
}

snode *skiplist_get_next(sk_iter* iter)
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
	for(int i = 0; i < MAX_L; i++)
		list->header->list[i] = list->header;
	list->header->key = INT_MAX;
}

void skiplist_free(skiplist *list)
{
	skiplist_clear(list);
	free(list->header->list);
	free(list->header);
	free(list);
	return ;
}
// for test
void skiplist_dump_key(skiplist * list)
{
	sk_iter *iter = skiplist_get_iterator(list);
	snode *now;
	while((now = skiplist_get_next(iter)) != NULL)
	{
		for(int i = 1; i <= now->level; i++)
			printf("%u ", now->key);
		printf("\n");
	}
	free(iter);
}
// for test
void skiplist_dump_key_value(skiplist * list)
{
	sk_iter *iter = skiplist_get_iterator(list);
	snode *now;
	while((now = skiplist_get_next(iter)) != NULL)
	{//수정하자 너무 이상함
		printf("key(%u): hexvalue1(0x%" PRIx64 "), hexvalue2(0x%" PRIx64 "),\
 hexvalue3(0x%" PRIx64 "), hexvalue4(0x%" PRIx64 "), erase(%d)\n", 
		now->key, now->VBM[0], now->VBM[1], now->VBM[2], now->VBM[3], now->erase);
	}
	free(iter);
}
