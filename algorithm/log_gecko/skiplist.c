#include "skiplist.h"

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
	if(list->size == MAX_PER_PAGE)
		skiplist_flush(list);
	return x;
}

int skiplist_delete(skiplist *list, KEYT key)
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
void skiplist_dump_key(skiplist *list)
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
void skiplist_dump_key_value(skiplist *list)
{
	sk_iter *iter = skiplist_get_iterator(list);
	snode *now;
	while((now = skiplist_get_next(iter)) != NULL)
	{
		printf("key(%u): hexvalue1(0x%" PRIx64 "), hexvalue2(0x%" PRIx64 "),\
 hexvalue3(0x%" PRIx64 "), hexvalue4(0x%" PRIx64 "), erase(%d)\n", 
		now->key, now->VBM[0], now->VBM[1], now->VBM[2], now->VBM[3], now->erase);
	}
	free(iter);
}

int skiplist_flush(skiplist *list)
{

}

value_set **skiplist_make_valueset(skiplist *input){
	value_set **res=(value_set**)malloc(sizeof(value_set*)*KEYNUM);
	memset(res,0,sizeof(value_set*)*KEYNUM);
	l_bucket b;
	memset(&b,0,sizeof(b));

	snode *target;
	sk_iter* iter=skiplist_get_iterator(input);
	int total_size=0;
	while((target=skiplist_get_next(iter))){
		b.bucket[target->value->length][b.idx[target->value->length]++]=target;
		total_size+=target->value->length;
	}
	free(iter);
	
	int res_idx=0;
	for(int i=0; i<b.idx[PAGESIZE/PIECE]; i++){
		snode *target=b.bucket[PAGESIZE/PIECE][i];
		res[res_idx]=target->value; //if target->value==PAGESIZE
		target->value=NULL;
		res[res_idx]->ppa=getDPPA(target->key,true);
		res_idx++;
	}

	b.idx[PAGESIZE/PIECE]=0;
	while(1){
		PTR page=NULL;
		int ptr=0;
		int remain=PAGESIZE-PIECE;
		footer *foot=f_init();

		res[res_idx]=inf_get_valueset(page,FS_MALLOC_W,PAGESIZE); //assign new dma in page
		res[res_idx]->ppa=getDPPA(0,false);
		page=res[res_idx]->value;

		while(remain>0){
			int target_length=remain/PIECE;
			while(b.idx[target_length]==0 && target_length!=0) --target_length;
			if(target_length==0){
				break;
			}
			target=b.bucket[target_length][b.idx[target_length]];
			target->ppa=res[res_idx]->ppa;
			f_insert(foot,target->key,target_length);

			memcpy(&page[ptr],target->value,target_length*PIECE);
			b.idx[target_length]--;

			ptr+=target_length*PIECE;
			remain-=target_length*PIECE;
		}
		memcpy(&page[(PAGESIZE/PIECE-1)*PIECE],foot,sizeof(footer));
		
		res_idx++;

		free(foot);
		bool stop=0;
		for(int i=0; i<PAGESIZE/PIECE; i++){
			if(b.idx[i]!=0)
				break;
			if(i==PAGESIZE/PIECE) stop=true;
		}
		if(stop) break;
	}

	return res;
}
