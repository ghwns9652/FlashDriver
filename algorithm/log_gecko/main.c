#include "skiplist.h"

//merge 할때 flag따라 무시될수있는지 확인 필요
//test set 재설정
//node에서 다시 불러올때 offset이 아닌 value값 자체가 들어갈 수 있게 함수 추가해야함
//key value 값이 제대로 들어갔는지 체크하는 함수 필요

#define INPUTSIZE 1024 //input size for DEBUG

int main()
{
	skiplist * temp = skiplist_init(); //make new skiplist
	uint8_t offset;
	for(int i = 0; i < INPUTSIZE; i++)
	{
		offset = i % 256;
		skiplist_insert(temp, i, offset, 0); //the value is copied
	}

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

	for(int i = INPUTSIZE; i < 2 * INPUTSIZE; i++)
	{
		offset = i % 256;
		skiplist_insert(temp, i, offset, 0); //the value is copied
	}

	//skiplist_dump_key(temp); //dump key and node's level
	skiplist_dump_key_value(temp); //dump key and value
	snode *finded = skiplist_find(temp, 6);
	printf("find : [%d]\n", finded->key);
	skiplist_free(temp);
	return 0;
}
