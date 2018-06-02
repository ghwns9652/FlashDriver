#include "gecko_skiplist.h"
#include "gecko_lsmtree.h"

#define DEB1
//#define DEB2
//#define DEB3

#define INPUTSIZE 15360 //input size for DEBUG
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
	//int set = rand() % 10;
	//printf("set %d\n",set);
	printf("update start\n");
	for(int i = 0; i < INPUTSIZE; i++)
		lsm_buf_update(temp, key[i], key[i] % 256, key[i] % 256 == 0? 1 : 0); //the value is copied
	printf("update end\n");
	print_level_status(temp);
	getchar();
	printf("dump start\n");
	getchar();
	printf("dump 1, 0\n");
	lsm_node_recover(temp, 1, 0);
	printf("dump end\n");

#endif

#ifdef DEB2
	printf("update start\n");
	for(int i = 0; i < INPUTSIZE; i++)
		lsm_buf_update(temp, i, 0, 0);
	printf("update end\n");
	printf("dump 1, 0\n");
	lsm_node_recover(temp, 1, 0);
	printf("dump 1, 1\n");
	lsm_node_recover(temp, 1, 1);
	printf("dump 2, 0\n");
	lsm_node_recover(temp, 2, 0);
	printf("dump 2, 1\n");
	lsm_node_recover(temp, 2, 1);
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
