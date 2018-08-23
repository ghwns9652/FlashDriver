#include "gecko.h"

#define INPUTSIZE 15360 //input size for DEBUG
#define INPUTTABLE 100000

int main(){
	printf("start\n");
	lsmtree* temp = lsm_init(); //make new skiplist
	exit(1);
	printf("lsm init done\n");
	uint32_t* key = (uint32_t*)malloc(sizeof(uint32_t) * INPUTTABLE);
	for(int i = 0; i < INPUTTABLE; i++){
		key[i] = rand() % 15360;
	}

	printf("update start\n");
	for(int i = 0; i < INPUTSIZE; i++){
		lsm_buf_update(temp, key[i], key[i] % 256, 0); //the value is copied
		//lsm_buf_update(temp, key[i], key[i] % 256, key[i] % 256 == 0? 1 : 0); //the value is copied
	}

	printf("update end\n");
	print_level_status(temp);
	getchar();
	printf("dump start\n");
	getchar();
	printf("dump 1, 0\n");
	lsm_node_recover(temp, 1, 0);
	printf("dump end\n");

	lsm_free(temp);
	free(key);
	return 0;
}
