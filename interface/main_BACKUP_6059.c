#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/settings.h"
#include "../include/types.h"
#include "interface.h"
#define INPUTSIZE 1000000
int main(){
	inf_init();
	srand(time(NULL));
	int key_save[INPUTSIZE];
	for(int i=0; i<INPUTSIZE; i++){
#ifdef LEAKCHECK
		printf("set: %d\n", i);
#endif
		int rand_key = rand()%20000;
		char *temp=(char*)malloc(PAGESIZE);
<<<<<<< HEAD
		memcpy(temp,&rand_key,sizeof(rand_key));
		key_save[i] = rand_key;
		inf_make_req(FS_SET_T,rand_key,temp);
		free(temp);
	}

	int check;
	for(int i=0; i<INPUTSIZE; i++){
		char *temp=(char*)malloc(PAGESIZE);
		inf_make_req(FS_GET_T,key_save[i],temp);
		memcpy(&check,temp,sizeof(key_save[i]));
		//printf("get:%d\n", check);
		free(temp);
=======
		memset(temp,0,PAGESIZE);
		memcpy(temp,&i,sizeof(i));
		inf_make_req(FS_SET_T,i,temp);
	}
	for(int i=0; i<300; i++){
		char *temp=(char*)malloc(PAGESIZE);
		memset(temp,0,PAGESIZE);
		inf_make_req(FS_GET_T,i,temp);
>>>>>>> 7b5dec2a4140d89362da452368518b233d0c77b2
	}

	inf_free();
}
