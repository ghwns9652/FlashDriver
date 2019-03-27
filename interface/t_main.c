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
#include "interface.h"

#define BLK_NUM 16
MeasureTime bt;
MeasureTime st;
float total_sec;
int write_cnt, read_cnt;

void log_print(int sig){
 	if (total_sec) {
	printf("\ntotal sec: %.2f\n", total_sec);
	printf("read throughput: %.2fMB/s\n", (float)read_cnt*8192/total_sec/1000/1000);
	printf("write throughput: %.2fMB/s\n\n", (float)write_cnt*8192/total_sec/1000/1000);
	}

	inf_free();
	exit(1);
}

int main(int argc, char *argv[]) {
 	int rw, len;
	int8_t fs_type;
	unsigned long long int offset;
	FILE *fp;

	char command[2];
	char type[5];
	double cal_len;
	struct sigaction sa;
	sa.sa_handler = log_print;
	sigaction(SIGINT, &sa, NULL);

	measure_init(&bt);
	measure_init(&st);

	inf_init();
	bench_init();
	bench_add(NOR,0,-1,-1,0);
	char t_value[PAGESIZE];
	memset(t_value, 'x', PAGESIZE);
	
	value_set dummy;
	dummy.value=t_value;
	dummy.dmatag=-1;
	dummy.length=PAGESIZE;

	fp = fopen(ROCKS_S_W_16, "r");
	if (fp == NULL) {
		printf("No file\n");
		return 1;
	}

	printf("%s load start!\n",ROCKS_S_W_16);
	while (fscanf(fp, "%s %s %llu %lf\n", command, type, &offset, &cal_len) != EOF) {
	 	static int cnt = 0;
		if(command[0] == 'D'){
			offset = offset / BLK_NUM;
			len = ceil(cal_len / BLK_NUM);
			if(type[0] == 'R'){
				fs_type = FS_GET_T;
			}else{
				fs_type = FS_SET_T;
			}
		}else{
			continue;
		}
		if(offset == 1805458)
			printf("offset = %d len = %d\n",offset,len);
		for (int i = 0; i < len; i++) {
			inf_make_req(fs_type, offset+i, t_value, PAGESIZE, 0);
			/*if (++cnt % 10240 == 0) {
				printf("cnt -- %d\n", cnt);
				printf("%d %llu %d\n", rw, offset, len);
			}*/
		}
	}
	printf("%s load complete!\n\n",ROCKS_S_W_16);
	fclose(fp);

	fp = fopen(ROCKS_SW_RR_16, "r");
	if (fp == NULL) {
		printf("No file\n");
		return 1;
	}

	MS(&bt);
	MS(&st);
	printf("%s bench start!\n", ROCKS_SW_RR_16);
	while (fscanf(fp, "%s %s %llu %lf\n", command, type, &offset, &cal_len) != EOF) {
	 	static int cnt = 0;
		if(command[0] == 'D'){
			offset = offset / BLK_NUM;
			len = ceil(cal_len / BLK_NUM);
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
			if (rw == 1) {
			 	write_cnt++;
			} else if (rw == 2) {
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
	}
	MA(&bt);
	printf("%s bench complete!\n",argv[1]);
	fclose(fp);

	total_sec = bt.adding.tv_sec + (float)bt.adding.tv_usec/1000000;

	printf("--------------- summary ------------------\n");
	printf("\ntotal sec: %.2f\n", total_sec);
	printf("read throughput: %.2fMB/s\n", (float)read_cnt*8192/total_sec/1000/1000);
	printf("write throughput: %.2fMB/s\n\n", (float)write_cnt*8192/total_sec/1000/1000);

	inf_free();

	return 0;
}

