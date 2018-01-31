#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/settings.h"
#include "../include/types.h"
#include "interface.h"
#define INPUTSIZE (TOTALSIZE * 100)
int main(){
	inf_init();
	srand(time(NULL));
	int key_save[INPUTSIZE];
	for(int i=0; i<INPUTSIZE; i++){
#ifdef LEAKCHECK
		printf("set: %d\n", i);
#endif
		int rand_key = rand()%(_NOP/2);
		char *temp=(char*)malloc(PAGESIZE);
		memcpy(temp,&rand_key,sizeof(rand_key));
		key_save[i] = rand_key;
		inf_make_req(FS_SET_T,rand_key,temp);
	}

	for(int i=0; i<INPUTSIZE; i++){
		char *temp=(char*)malloc(PAGESIZE);
		inf_make_req(FS_GET_T,key_save[i],temp);
	}

	inf_free();
}
