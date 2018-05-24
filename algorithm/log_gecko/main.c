#include "gecko_skiplist.h"
#include "gecko_lsmtree.h"

//#define DEB1
#define DEB2
//#define DEB3

#define INPUTSIZE 226304 //15360 //input size for DEBUG
#define INPUTTABLE 100000

int main()
{
	printf("start\n");
	lsmtree* temp = lsm_init(); //make new skiplist
	printf("lsm init done\n");
	uint32_t* key = (uint32_t*)malloc(sizeof(uint32_t)*INPUTTABLE);
	for(int i = 0; i < INPUTTABLE; i++)
		key[i] = rand() % 15360;

#ifdef DEB1
	int set = rand() % 10;
	printf("set %d\n",set);
	for(int i = INPUTSIZE * set; i < INPUTSIZE * (set + 1); i++)
		skiplist_insert(temp, i, 0, 0); //the value is copied
#endif

#ifdef DEB2
	printf("update start\n");
	for(int i = 0; i < INPUTSIZE; i++)
		lsm_buf_update(temp, i / 256, i % 256, 0);
	printf("update end\n");
	printf("dump 0, 0\n");
	lsm_node_recover(temp, 0, 0);
	printf("dump 0, 1\n");
	lsm_node_recover(temp, 0, 1);
	printf("dump 0, 2\n");
	lsm_node_recover(temp, 0, 2);
	printf("dump end\n");
#endif

#ifdef DEB3
	for(int i = INPUTSIZE * set; i < INPUTSIZE * (set + 1); i++)
	{
		snode *finded = skiplist_find(temp, i);
		if(finded == NULL)
			printf("***************\nERROR! key %d is not exist!\n***************\n", i);
	}
#endif

	lsm_free(temp);
	free(key);
	return 0;
}
