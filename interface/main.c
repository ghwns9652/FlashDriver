#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/settings.h"
#include "../include/types.h"
#include "interface.h"


#define	numKey	3000

int main(){
	inf_init();

	/* 1D array for Random key */
	//KEYT keyArray[300];
	printf("Start!\n");
	KEYT* keyArray = (KEYT*)malloc(sizeof(KEYT) * numKey);

	for(int i=0; i<numKey; i++){
#ifdef LEAKCHECK
		printf("set: %d\n",i);
#endif
		//printf("1\n");
		const KEYT new_key=rand() % 10;
		//printf("2\n");
		keyArray[i] = new_key;
		printf("Set_random: %d\n", keyArray[i]);
		//printf("3\n");
		char *temp = (char*)malloc(PAGESIZE);
		memcpy(temp, &new_key, sizeof(i));
		//printf("4\n");
		inf_make_req(FS_SET_T, i, temp); // error!
		//printf("5\n");
		free(temp);
		//printf("6\n");

		/* Followings are NORMAL INPUT */
		/*
		char *temp=(char*)malloc(PAGESIZE);
		memcpy(temp,&i,sizeof(i));
		inf_make_req(FS_SET_T,i,temp);
		free(temp);
		*/
	}

	int check;
	for(int i=0; i<numKey; i++){
		char *temp = (char*)malloc(PAGESIZE);
		inf_make_req(FS_GET_T, i, temp);
		memcpy(&check, temp, sizeof(i));
		//printf("get:%d\n", check);
		free(temp);

		/* Follwoings are NORMAL INPUT(sequential) */
		/*
		char *temp=(char*)malloc(PAGESIZE);
		inf_make_req(FS_GET_T,i,temp);
		memcpy(&check,temp,sizeof(i));
		printf("get:%d\n",check);
		free(temp);
		*/
	}
	free(keyArray);
	inf_free();
}
