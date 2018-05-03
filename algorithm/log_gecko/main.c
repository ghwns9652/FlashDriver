#include "skiplist.h"

#define INPUTSIZE 1024 //input size for DEBUG

int main()
{
	skiplist * temp = skiplist_init(); //make new skiplist
	uint8_t offset;
	bool flg = false;
	for(int i = 0; i < INPUTSIZE; i++)
	{
		offset = i % 256;
		skiplist_insert(temp, i / 256, offset, flg); //the value is copied
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
		skiplist_insert(temp, i / 256, offset, flg); //the value is copied
	}

	skiplist_dump(temp); //dump key and node's level
	snode *finded = skiplist_find(temp, 6);
	printf("find : [%d]\n", finded->key);
	skiplist_free(temp);
	return 0;
}
