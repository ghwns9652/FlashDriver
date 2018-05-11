#include "skiplist.h"

#define DEB1
//#define DEB2
#define DEB3

//merge 할때 flag따라 무시될수있는지 확인 필요
//node에서 다시 불러올때 offset이 아닌 value값 자체가 들어갈 수 있게 함수 추가해야함
//flush하는 함수 추가 필요
//insert 이후 flush 하는 함수 삽입 필요

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
	for(int i = 0; i < INPUTSIZE; i++)
		skiplist_insert(temp, i / 256, i % 256, 0); //the value is copied
#endif

/*
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
*/

	//skiplist_dump_key(temp); //dump key and node's level
	//skiplist_dump_key_value(temp); //dump key and value
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
