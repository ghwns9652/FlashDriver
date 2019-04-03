#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <math.h>
#include "../include/lsm_settings.h"
#include "../include/FS.h"
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "../bench/measurement.h"
#include "interface.h"

#define LOAD_FILE ROCKS_S_W_16
#define RUN_FILE  ROCKS_SW_RR_16
#define BLK_NUM 16
MeasureTime *bt;
MeasureTime *st;
float total_sec;
int write_cnt, read_cnt;
int32_t total_cnt;
/*
void log_print(int sig){
 	if (total_sec) {
	printf("\ntotal sec: %.2f\n", total_sec);
	printf("read throughput: %.2fMB/s\n", (float)read_cnt*8192/total_sec/1000/1000);
	printf("write throughput: %.2fMB/s\n\n", (float)write_cnt*8192/total_sec/1000/1000);
	}

	inf_free();
	exit(1);
}
*/
int main(int argc, char *argv[]) {
 	int len;
	int8_t fs_type;
	unsigned long long int offset;
	FILE *fp;

	char command[2];
	char type[5];
	double cal_len;
	struct sigaction sa;
	//sa.sa_handler = log_print;
	//sigaction(SIGINT, &sa, NULL);
	bt = (MeasureTime *)malloc(sizeof(MeasureTime));
	st = (MeasureTime *)malloc(sizeof(MeasureTime));
	measure_init(bt);
	measure_init(st);

	inf_init();
	bench_init();
	bench_add(NOR,0,-1,-1,0);
	char t_value[PAGESIZE];
	memset(t_value, 'x', PAGESIZE);
	/*	
	value_set dummy;
	dummy.value=t_value;
	dummy.dmatag=-1;
	dummy.length=PAGESIZE;
	*/
	
	printf("%s load start!\n",LOAD_FILE);
	fp = fopen(LOAD_FILE, "r");
	if (fp == NULL) {
		printf("No file\n");
		return 1;
	}

	while (fscanf(fp, "%s %s %llu %lf", command, type, &offset, &cal_len) != EOF) {
	 	static int cnt = 0;
		//printf("cnt = %d\n",cnt++);
		/*
		if(offset == 28887332){
			printf("%s %s %llu %lf\n",command, type, offset, cal_len);
			sleep(1);
		}
		*/
		if(command[0] == 'D'){
			offset = offset / BLK_NUM;
			len = ceil(cal_len / BLK_NUM);
			if(offset + len > RANGE){
				continue;
			}
			if(type[0] == 'R'){
				fs_type = FS_GET_T;
			}else{
				fs_type = FS_SET_T;
			}
		}else{
			continue;
		}
		
		for (int i = 0; i < len; i++) {
			inf_make_req(fs_type, offset+i, t_value, PAGESIZE, 0);
			/*if (++cnt % 10240 == 0) {
				printf("cnt -- %d\n", cnt);
				printf("%d %llu %d\n", rw, offset, len);
			}*/
		}
		memset(command,0,sizeof(char) * 2);
		memset(type,0,sizeof(char)*5);
		fflush(stdout);
	}
	printf("%s load complete!\n\n",LOAD_FILE);
	fclose(fp);
	printf("%s bench start!\n", RUN_FILE);
	fp = fopen(RUN_FILE, "r");
	if (fp == NULL) {
		printf("No file\n");
		return 1;
	}
	measure_start(bt);	
	
	while (fscanf(fp, "%s %s %llu %lf", command, type, &offset, &cal_len) != EOF) {
	 	//static int cnt = 0;
		if(command[0] == 'D'){
			offset = offset / BLK_NUM;
			len = ceil(cal_len / BLK_NUM);
			if(offset + len > RANGE){
				continue;
			}
			if(type[0] == 'R'){
				fs_type = FS_GET_T;
			}else{
				fs_type = FS_SET_T;
			}
		}else{
			continue;
		}
		for (int i = 0; i < len; i++) {
			inf_make_req(fs_type, offset+i, t_value, PAGESIZE, 1);
			if (fs_type == FS_SET_T) {
			 	write_cnt++;
			} else if (fs_type == FS_GET_T) {
			 	read_cnt++;
			}

			/*if (++cnt % 10240 == 0) {
			 	MA(&st);
				total_sec = st.adding.tv_sec + (float)st.adding.tv_usec/1000000;
	  			printf("\ntotal sec: %.2f\n", total_sec);
	 			printf("read throughput: %.2fMB/s\n", (float)read_cnt*8192/total_sec/1000/1000);
				printf("write throughput: %.2fMB/s\n\n", (float)write_cnt*8192/total_sec/1000/1000);
				MS(&st);
			}*/
		}
		memset(command,0,sizeof(char) * 2);
		memset(type,0,sizeof(char)*5);
		fflush(stdout);

	}
	measure_adding(bt);
	printf("%s bench complete!\n",RUN_FILE);
	fclose(fp);

	total_sec = bt->adding.tv_sec + (float)bt->adding.tv_usec/1000000;
	total_cnt = read_cnt + write_cnt;
	printf("--------------- summary ------------------\n");
	printf("\ntotal sec: %.2f\n", total_sec);
	printf("read_cnt : %d write_cnt : %d\n",read_cnt, write_cnt);
	printf("read throughput: %.2fMB/s\n", (float)read_cnt*8192/total_sec/1000/1000);
	printf("write throughput: %.2fMB/s\n\n", (float)write_cnt*8192/total_sec/1000/1000);
	printf("total_throughput: %.2fMB/s\n",(float)total_cnt*8192/total_sec/1000/1000);
	free(bt);
	free(st);
	inf_free();

	return 0;
}

