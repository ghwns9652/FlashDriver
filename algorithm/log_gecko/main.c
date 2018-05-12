#include "skiplist.h"

//#define DEB1
#define DEB2
//#define DEB3
#define DEB4

#define INPUTSIZE 10000 //input size for DEBUG
#define INPUTTABLE 100000

int main()
{
	skiplist * temp = skiplist_init(); //make new skiplist
	uint32_t* key = (uint32_t*)malloc(sizeof(uint32_t)*INPUTTABLE);
	for(int i = 0; i < INPUTTABLE; i++)
		key[i] = rand() % INT_MAX;
#ifdef DEB1
	int set = rand() % 10;
	printf("set %d\n",set);
	for(int i = INPUTSIZE * set; i < INPUTSIZE * (set + 1); i++)
		skiplist_insert(temp, i, 0, 0); //the value is copied
#endif

#ifdef DEB2
	for(int i = 0; i < 200; i++)
		skiplist_insert(temp, i, 0, 0); //the value is copied
#endif

#ifdef DEB4
	snode *node;
	int cnt = 0;
	while(temp->size != 0)
	{
		sk_iter *iter = skiplist_get_iterator(temp); //read only iterator
		while((node = skiplist_get_next(iter)) != NULL)
		{ 
			if(node->level == temp->level)
			{
				skiplist_delete(temp, node->key); //if iterator's now node is deleted, can't use the iterator! 
												  //should make new iterator
				cnt++;
				break;
			}
		}
		free(iter); //must free iterator 
		if(cnt == 10)
			break;
	}
	skiplist_dump_key(temp);
#endif

#ifdef DEB3
	for(int i = INPUTSIZE * set; i < INPUTSIZE * (set + 1); i++)
	{
		snode *finded = skiplist_find(temp, i);
		if(finded == NULL)
			printf("***************\nERROR! key %d is not exist!\n***************\n", i);
	}
#endif

	skiplist_free(temp);
	free(key);
	return 0;
}
