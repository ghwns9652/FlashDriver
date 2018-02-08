#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "interface.h"


#define	numKey	1000

int main(){
	bench_init(2);
	bench_add(SEQSET,0,1024*30,1024*30);
	bench_add(SEQGET,0,1024*30,1024*30);
	inf_init();
#if 0
	<<<<<<< HEAD

	/* 1D array for Random key */
	//KEYT keyArray[300];
	printf("Start!\n");
	KEYT* keyArray = (KEYT*)malloc(sizeof(KEYT) * numKey);

	for(int i=0; i<numKey; i++){
=======
#endif
	bench_value *value;
	while((value=get_bench())){
		char *data=(char*)malloc(PAGESIZE);
		memset(data,0,PAGESIZE);
		if(value->type==FS_SET_T){
			memcpy(data,&value->key,sizeof(value->key));
		}
#ifdef BENCH
		inf_make_req(value->type,value->key,data,value->mark);
#else
		inf_make_req(value->type,value->key,data);
#endif
	}
	
	while(!bench_is_finish()){
#ifdef LEAKCHECK
		sleep(1);
#endif
	}
	bench_print();
	bench_free();
	inf_free();
/*
:	for(int i=0; i<1024*2; i++){
>>>>>>> 15f3b6e8d6807de47d50b7c5ae4c8fdb2fabbb99
#ifdef LEAKCHECK
		//printf("set: %d\n",i);
#endif
		//printf("1\n");
		const KEYT new_key = rand() % 50;
		//printf("2\n");
		keyArray[i] = new_key;
		//printf("Set_random: %d\n", keyArray[i]);
		//printf("3\n");
#ifdef LEAKCHECK
		printf("set_random: %d\n", keyArray[i]);
#endif

#if 0
		char *temp = (char*)malloc(PAGESIZE);
		memcpy(temp, &new_key, sizeof(i));
		//printf("4\n");
		inf_make_req(FS_SET_T, i, temp); // error!
		//printf("5\n");
		free(temp);
		//printf("6\n");
#endif

		/* Followings are NORMAL INPUT */
		
		char *temp=(char*)malloc(PAGESIZE);
		memset(temp,0,PAGESIZE);
		//memcpy(temp,&i,sizeof(i));
		memcpy(temp, &new_key, sizeof(i));
		inf_make_req(FS_SET_T,i,temp);
//<<<<<<< HEAD
		//free(temp);
		
	}

<<<<<<< HEAD
	int check;
	for(int i=0; i<numKey; i++){
		
#if 0
		char *temp = (char*)malloc(PAGESIZE);
		inf_make_req(FS_GET_T, i, temp);
		memcpy(&check, temp, sizeof(i));
		//printf("get:%d\n", check);
		free(temp);

#endif

		/* Follwoings are NORMAL INPUT(sequential) */
		
//=======
	
	//for(int i=0; i<300; i++){
//>>>>>>> intern/master
		char *temp=(char*)malloc(PAGESIZE);
		memset(temp,0,PAGESIZE);
		inf_make_req(FS_GET_T,i,temp);
//<<<<<<< HEAD
		//memcpy(&check,temp,sizeof(i));
		//printf("get:%d\n",check);

		//free(temp);
		
//=======
//>>>>>>> intern/master
	}
	free(keyArray);
	inf_free();
=======
	for(int i=0; i<1024*2; i++){
		char *temp=(char*)malloc(PAGESIZE);
		memset(temp,0,PAGESIZE);
		inf_make_req(FS_GET_T,i,temp);
	}
 */
	return 0;
>>>>>>> 15f3b6e8d6807de47d50b7c5ae4c8fdb2fabbb99
}
