--------------------------settings.h-----------------------------
#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>
#define ASYNC 1

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

#ifdef MLC

#define TOTALSIZE (300L*G)
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS ((_PPS)/_PPB)

#elif defined(SLC)

#define TOTALSIZE (10L*G)
//#define REALSIZE (512L*G)
//#define TOTALSIZE (128*M) // 1 Segment(16384 Page) 하나마다 TOTALSIZE 128 M만큼 필요
#define REALSIZE (256L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS (64)

#endif

#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOP (TOTALSIZE/PAGESIZE)
#define _NOS (TOTALSIZE/(_PPS*PAGESIZE))
#define _NOB (BPS*_NOS)
#define _RNOS (REALSIZE/(_PPS*PAGESIZE))//real number of segment
//#define RANGE (1*128*1024L) // 1 G
#define RANGE (1*128*32) // 32 M

#define FSTYPE uint8_t
#define KEYT uint32_t
#define BLOCKT uint32_t
#define OOBT uint64_t
#define V_PTR char * const
#define PTR char*
#define QSIZE (1024)
#define THREADSIZE (1)

#ifndef __GNUG__
typedef enum{false,true} bool;
#endif

typedef enum{
	SEQGET,SEQSET,
	RANDGET,RANDSET,
	RANDRW,SEQRW,
	MIXED
}bench_type;
#endif
----------------------------main.c-------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "../include/lsm_settings.h"
#include "../include/FS.h"
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "interface.h"
int main(){/*
>>>>>>> 5c776eb8c03af5769e5d04f2343aebeb491e8220
	int Input_cycle;
	int Input_type;
	int start;
	int end;
	int Input_size;
	printf("How many times would you run a benchmark?");
	scanf("%d", &Input_cycle);
	bench_init(Input_cycle);
	printf("please type the bench_type, start, end and input size\n");
	printf("====bench type list====\n");
	printf("SEQSET = 1, \nSEQGET = 2, \nRANDGET = 3, \nRANDRW = 4, \n");
	printf("SEQRW = 5, \nRANDSET = 6, \nMIXED = 7\n");
	printf("====bench type list end ====\n");
   printf("ex. 1 0 100 100 means seqset 0 to 100 with input size 100\n");
	for (int i = 0; i < Input_cycle; i++)
	{
		scanf("%d %d %d %d",&Input_type, &start, &end, &Input_size);
		if(Input_type == 1)
			bench_add(SEQSET,start,end,Input_size);
		else if(Input_type == 2)
			bench_add(SEQGET,start,end,Input_size);
		else if(Input_type == 3)
			bench_add(RANDGET,start,end,Input_size);
		else if(Input_type == 4)
			bench_add(RANDRW,start,end,Input_size);
		else if(Input_type == 5)
			bench_add(SEQRW,start,end,Input_size);
		else if(Input_type == 6)
			bench_add(RANDSET,start,end,Input_size);
		else if(Input_type == 7)
			bench_add(MIXED,start,end,Input_size);
		else{
			printf("invalid setting input is detected. please rerun the bench!\n");
			return 0;
		}
		printf("benchmark # %d is initiailized.\n",i+1);
		
		if( i == Input_cycle -1)
			printf("initilization done.\n");
		else
			printf("please type in next benchmark settings.\n");

	}

	printf("benchmark setting done. starts now.\n");
*/

	inf_init();
#define SEQ
#ifdef SEQ
	bench_init(2);
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	int a=128L*K, b=128L*K;
	bench_add(SEQSET,0,a,a+20);
	bench_add(SEQGET,0,a,a+20);
	//bench_add(SEQSET,0,1L*M,2L*M);
	//bench_add(SEQGET,0,1L*M,2L*M);
	//bench_add(RANDSET,0,1*1024,1*1024);
	//bench_add(RANDGET,0,1*1024,1*1024);
	//bench_add(RANDRW,0,16*1024,64*1024);
#endif
#ifdef RAND
	bench_init(1);
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	//bench_add(SEQSET,0,1*10,1*10);
	//bench_add(SEQGET,0,1*10,1*10);
	//bench_add(SEQSET,0,16*1024,32*1024);
	//bench_add(SEQGET,0,16*1024,32*1024);
	//bench_add(RANDSET,0,1*1024,1*1024);
	//bench_add(RANDGET,0,1*1024,1*1024);
	//bench_add(RANDRW,0,16*1024,32*1024);
	bench_add(RANDRW,0,200*RANGE,RANGE);
#endif

	/*
	for(int i=0; i<PAGESIZE;i++){
		t_value2[i]=rand()%256;
	}*/
//	bench_add(RANDSET,0,15*1024,15*1024);
//	bench_add(RANDGET,0,15*1024,15*1024);
	bench_value *value;

	value_set temp;
	temp.value=t_value;
	//temp.value=NULL;
	temp.dmatag=-1;
	temp.length=0;
	int cnt=0;
	while((value=get_bench())){
		temp.length=value->length;
		inf_make_req(value->type,value->key,&temp,value->mark);
		cnt++;
	}
	
	while(!bench_is_finish()){
#ifdef LEAKCHECK
		sleep(1);
#endif
	}
	bench_print();

	bench_free();
	inf_free();
	return 0;
}
-----------------------simaultor result-------------------------------
mutex_t : 0xddc068 q:0xddc060, size:1024
mutex_t : 0xddd418 q:0xddd410, size:1024
block_create start!
BM_Init() End!
block_create end!
TOTALSIZE: 10737418240
REALSIZE: 274877906944
PAGESIZE: 8192
_PPB: 256, ppb = 16384, _PPS: 16384, BPS: 64
BLOCKSIZE: 2097152
_NOP: 1310720
_NOS: 80
_NOB: 5120
_RNOS: 2048
RANGE: 4096
async: 1
_nos:80

badblock_cnt: 68
_RNOS:2048
bad block 753664 -> 33538048
bad block 1179648 -> 33521664
bad block 1474560 -> 33505280
bad block 1490944 -> 33488896
bad block 1736704 -> 33472512
bad block 1900544 -> 33456128
bad block 2555904 -> 33439744
checking done!
making seq Set bench!
 testing...... [0%] testing...... [1%] testing...... [2%] testing...... [3%] testing...... [4%] testing...... [5%] testing...... [6%] testing...... [7%] testing...... [8%] testing...... [9%] testing...... [10%] testing...... [11%] testing...... [12%] testing...... [13%] testing...... [14%] testing...... [15%] testing...... [16%] testing...... [17%] testing...... [18%] testing...... [19%] testing...... [20%] testing...... [21%] testing...... [22%] testing...... [23%] testing...... [24%] testing...... [25%] testing...... [26%] testing...... [27%] testing...... [28%] testing...... [29%] testing...... [30%] testing...... [31%] testing...... [32%] testing...... [33%] testing...... [34%] testing...... [35%] testing...... [36%] testing...... [37%] testing...... [38%] testing...... [39%] testing...... [40%] testing...... [41%] testing...... [42%] testing...... [43%] testing...... [44%] testing...... [45%] testing...... [46%] testing...... [47%] testing...... [48%] testing...... [49%] testing...... [50%] testing...... [51%] testing...... [52%] testing...... [53%] testing...... [54%] testing...... [55%] testing...... [56%] testing...... [57%] testing...... [58%] testing...... [59%] testing...... [60%] testing...... [61%] testing...... [62%] testing...... [63%] testing...... [64%] testing...... [65%] testing...... [66%] testing...... [67%] testing...... [68%] testing...... [69%] testing...... [70%] testing...... [71%] testing...... [72%] testing...... [73%] testing...... [74%] testing...... [75%] testing...... [76%] testing...... [77%] testing...... [78%] testing...... [79%] testing...... [80%] testing...... [81%] testing...... [82%] testing...... [83%] testing...... [84%] testing...... [85%] testing...... [86%] testing...... [87%] testing...... [88%] testing...... [89%] testing...... [90%] testing...... [91%] testing...... [92%] testing...... [93%] testing...... [94%] testing...... [95%] testing...... [96%] testing...... [97%] testing...... [98%] testing...... [99%] testing...... [100%]testing...... [100%] done!

making seq Get bench!
 testing...... [0%] testing...... [1%] testing...... [2%] testing...... [3%] testing...... [4%] testing...... [5%] testing...... [6%] testing...... [7%] testing...... [8%] testing...... [9%] testing...... [10%] testing...... [11%] testing...... [12%] testing...... [13%] testing...... [14%] testing...... [15%] testing...... [16%] testing...... [17%] testing...... [18%] testing...... [19%] testing...... [20%] testing...... [21%] testing...... [22%] testing...... [23%] testing...... [24%] testing...... [25%] testing...... [26%] testing...... [27%] testing...... [28%] testing...... [29%] testing...... [30%] testing...... [31%] testing...... [32%] testing...... [33%] testing...... [34%] testing...... [35%] testing...... [36%] testing...... [37%] testing...... [38%] testing...... [39%] testing...... [40%] testing...... [41%] testing...... [42%] testing...... [43%] testing...... [44%] testing...... [45%] testing...... [46%] testing...... [47%] testing...... [48%] testing...... [49%] testing...... [50%] testing...... [51%] testing...... [52%] testing...... [53%] testing...... [54%] testing...... [55%] testing...... [56%] testing...... [57%] testing...... [58%] testing...... [59%] testing...... [60%] testing...... [61%] testing...... [62%] testing...... [63%] testing...... [64%] testing...... [65%] testing...... [66%] testing...... [67%] testing...... [68%] testing...... [69%] testing...... [70%] testing...... [71%] testing...... [72%] testing...... [73%] testing...... [74%] testing...... [75%] testing...... [76%] testing...... [77%] testing...... [78%] testing...... [79%] testing...... [80%] testing...... [81%] testing...... [82%] testing...... [83%] testing...... [84%] testing...... [85%] testing...... [86%] testing...... [87%] testing...... [88%] testing...... [89%] testing...... [90%] testing...... [91%] testing...... [92%] testing...... [93%] testing...... [94%] testing...... [95%] testing...... [96%] testing...... [97%] testing...... [98%] testing...... [99%] testing...... [100%]testing...... [100%] done!




[cdf]read---
--------------------------------------------
|            bench type:                   |
--------------------------------------------
----algorithm----
[0~100(usec)]: 131092
[100~200(usec)]: 0
[200~300(usec)]: 0
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----lower----
[0~100(usec)]: 130668
[100~200(usec)]: 5
[200~300(usec)]: 0
[300~400(usec)]: 1
[400~500(usec)]: 1
[500~600(usec)]: 1
[600~700(usec)]: 1
[700~800(usec)]: 0
[800~900(usec)]: 3
[900~1000(usec)]: 8
[over_ms]: 404
[over__s]: 0
----average----
[avg_algo]: 0.0
[avg_low]: 0.66
-----lower_info----
[write_op]: 442388
[read_op]: 327660
[trim_op]:19
[WAF, RAF]: 3.374638, 2.499466
[if rw test]: 6.749275(WAF), 4.998932(RAF)
[all write Time]:9.386055
[all read Time]:0.997183

----summary----
[all_time]: 12.696415
[size]: 1024.156250(mb)
[FAIL NUM] 0
[SUCCESS RATIO] 1.000000
[throughput] 82600.954679(kb/s)
             80.664995(mb/s)
[READ WRITE CNT] 0 131092



[cdf]read---
0		9		0.000069
1		13		0.000168
2		14		0.000275
3		9		0.000343
4		17		0.000473
5		16		0.000595
6		14		0.000702
7		17		0.000831
8		13		0.000931
9		13		0.001030
10		15		0.001144
11		19		0.001289
12		17		0.001419
13		17		0.001549
14		7		0.001602
15		15		0.001716
16		5		0.001754
17		15		0.001869
18		10		0.001945
19		14		0.002052
20		19		0.002197
21		21		0.002357
22		11		0.002441
23		18		0.002578
24		15		0.002693
25		12		0.002784
26		10		0.002861
27		11		0.002944
28		20		0.003097
29		14		0.003204
30		16		0.003326
31		19		0.003471
32		21		0.003631
33		20		0.003784
34		20		0.003936
35		15		0.004051
36		19		0.004196
37		20		0.004348
38		20		0.004501
39		17		0.004630
40		40		0.004935
41		51		0.005325
42		63		0.005805
43		788		0.011816
44		2542		0.031207
45		34263		0.292573
46		35853		0.566068
47		14027		0.673069
48		13161		0.773464
49		6968		0.826618
50		9571		0.899628
51		4493		0.933901
52		2659		0.954185
53		2566		0.973759
54		1094		0.982104
55		347		0.984751
56		265		0.986773
57		170		0.988069
58		150		0.989214
59		171		0.990518
60		165		0.991777
61		82		0.992402
62		96		0.993135
63		115		0.994012
64		97		0.994752
65		115		0.995629
66		88		0.996300
67		106		0.997109
68		89		0.997788
69		86		0.998444
70		103		0.999230
71		58		0.999672
72		43		1.000000
--------------------------------------------
|            bench type:                   |
--------------------------------------------
----algorithm----
[0~100(usec)]: 131092
[100~200(usec)]: 0
[200~300(usec)]: 0
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----lower----
[0~100(usec)]: 131092
[100~200(usec)]: 0
[200~300(usec)]: 0
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----average----
[avg_algo]: 0.0
[avg_low]: 0.2
-----lower_info----
[write_op]: 16364
[read_op]: 131092
[trim_op]:1
[WAF, RAF]: 0.124828, 1.000000
[if rw test]: 0.249657(WAF), 2.000000(RAF)
[all write Time]:0.941669
[all read Time]:0.388752

----summary----
[all_time]: 1.633674
[size]: 1024.156250(mb)
[FAIL NUM] 0
[SUCCESS RATIO] 1.000000
[throughput] 641949.373008(kb/s)
             626.903685(mb/s)
[READ WRITE CNT] 131092 0
result of ms:
---
bye bye!
--------------------------settings.h-----------------------------
#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>
#define ASYNC 1

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

#ifdef MLC

#define TOTALSIZE (300L*G)
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS ((_PPS)/_PPB)

#elif defined(SLC)

#define TOTALSIZE (10L*G)
//#define REALSIZE (512L*G)
//#define TOTALSIZE (128*M) // 1 Segment(16384 Page) 하나마다 TOTALSIZE 128 M만큼 필요
#define REALSIZE (256L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS (64)

#endif

#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOP (TOTALSIZE/PAGESIZE)
#define _NOS (TOTALSIZE/(_PPS*PAGESIZE))
#define _NOB (BPS*_NOS)
#define _RNOS (REALSIZE/(_PPS*PAGESIZE))//real number of segment
//#define RANGE (1*128*1024L) // 1 G
#define RANGE (10*128*1024L) // 32 M

#define FSTYPE uint8_t
#define KEYT uint32_t
#define BLOCKT uint32_t
#define OOBT uint64_t
#define V_PTR char * const
#define PTR char*
#define QSIZE (1024)
#define THREADSIZE (1)

#ifndef __GNUG__
typedef enum{false,true} bool;
#endif

typedef enum{
	SEQGET,SEQSET,
	RANDGET,RANDSET,
	RANDRW,SEQRW,
	MIXED
}bench_type;
#endif
----------------------------main.c-------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "../include/lsm_settings.h"
#include "../include/FS.h"
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "interface.h"
extern int req_cnt_test;
extern uint64_t dm_intr_cnt;
int main(){/*
>>>>>>> 5c776eb8c03af5769e5d04f2343aebeb491e8220
	int Input_cycle;
	int Input_type;
	int start;
	int end;
	int Input_size;
	printf("How many times would you run a benchmark?");
	scanf("%d", &Input_cycle);
	bench_init(Input_cycle);
	printf("please type the bench_type, start, end and input size\n");
	printf("====bench type list====\n");
	printf("SEQSET = 1, \nSEQGET = 2, \nRANDGET = 3, \nRANDRW = 4, \n");
	printf("SEQRW = 5, \nRANDSET = 6, \nMIXED = 7\n");
	printf("====bench type list end ====\n");
   printf("ex. 1 0 100 100 means seqset 0 to 100 with input size 100\n");
	for (int i = 0; i < Input_cycle; i++)
	{
		scanf("%d %d %d %d",&Input_type, &start, &end, &Input_size);
		if(Input_type == 1)
			bench_add(SEQSET,start,end,Input_size);
		else if(Input_type == 2)
			bench_add(SEQGET,start,end,Input_size);
		else if(Input_type == 3)
			bench_add(RANDGET,start,end,Input_size);
		else if(Input_type == 4)
			bench_add(RANDRW,start,end,Input_size);
		else if(Input_type == 5)
			bench_add(SEQRW,start,end,Input_size);
		else if(Input_type == 6)
			/bench_add(RANDSET,start,end,Input_size);
		else if(Input_type == 7)
			bench_add(MIXED,start,end,Input_size);
		else{
			printf("invalid setting input is detected. please rerun the bench!\n");
			return 0;
		}
		printf("benchmark # %d is initiailized.\n",i+1);
		
		if( i == Input_cycle -1)
			printf("initilization done.\n");
		else
			printf("please type in next benchmark settings.\n");

	}

	printf("benchmark setting done. starts now.\n");
*/

	inf_init();
#define SEQ
#ifdef SEQ
	bench_init(2);
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	int a=128L*K, b=128L*K;
	bench_add(SEQSET,0,a,a+20);
	bench_add(SEQGET,0,a,a+20);
	//bench_add(SEQSET,0,1L*M,2L*M);
	//bench_add(SEQGET,0,1L*M,2L*M);
	//bench_add(RANDSET,0,1*1024,1*1024);
	//bench_add(RANDGET,0,1*1024,1*1024);
	//bench_add(RANDRW,0,16*1024,64*1024);
#endif
#ifdef RAND
	bench_init(1);
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	//bench_add(SEQSET,0,1*10,1*10);
	//bench_add(SEQGET,0,1*10,1*10);
	//bench_add(SEQSET,0,16*1024,32*1024);
	//bench_add(SEQGET,0,16*1024,32*1024);
	//bench_add(RANDSET,0,1*1024,1*1024);
	//bench_add(RANDGET,0,1*1024,1*1024);
	//bench_add(RANDRW,0,16*1024,32*1024);
	bench_add(RANDRW,0,200*RANGE,RANGE);
#endif

	/*
	for(int i=0; i<PAGESIZE;i++){
		t_value2[i]=rand()%256;
	}*/
//	bench_add(RANDSET,0,15*1024,15*1024);
//	bench_add(RANDGET,0,15*1024,15*1024);
	bench_value *value;

	value_set temp;
	temp.value=t_value;
	//temp.value=NULL;
	temp.dmatag=-1;
	temp.length=0;
	int cnt=0;
	while((value=get_bench())){
		temp.length=value->length;
		inf_make_req(value->type,value->key,&temp,value->mark);
		cnt++;
	}

	if(req_cnt_test==cnt){
		printf("dpne!\n");
	}
	else{
		printf("req_cnt_test:cnt -> %d:%d fuck\n",req_cnt_test,cnt);
	}

	while(!bench_is_finish()){
#ifdef LEAKCHECK
		sleep(1);
#endif
	}
	bench_print();

	bench_free();
	inf_free();
	return 0;
}
-----------------------simaultor result-------------------------------
mutex_t : 0x1842068 q:0x1842060, size:1024
!!! posix memory ASYNC: 1!!!
mutex_t : 0x18436f8 q:0x18436f0, size:1024
block_create start!
BM_Init() End!
block_create end!
TOTALSIZE: 10737418240
REALSIZE: 274877906944
PAGESIZE: 8192
_PPB: 256, ppb = 16384, _PPS: 16384, BPS: 64
BLOCKSIZE: 2097152
_NOP: 1310720
_NOS: 80
_NOB: 5120
_RNOS: 2048
RANGE: 1310720
async: 1
_nos:80
making seq Set bench!
 testing...... [0%] testing...... [1%] testing...... [2%] testing...... [3%] testing...... [4%] testing...... [5%] testing...... [6%] testing...... [7%] testing...... [8%] testing...... [9%] testing...... [10%] testing...... [11%] testing...... [12%] testing...... [13%] testing...... [14%] testing...... [15%] testing...... [16%] testing...... [17%] testing...... [18%] testing...... [19%] testing...... [20%] testing...... [21%] testing...... [22%] testing...... [23%] testing...... [24%] testing...... [25%] testing...... [26%] testing...... [27%] testing...... [28%] testing...... [29%] testing...... [30%] testing...... [31%] testing...... [32%] testing...... [33%] testing...... [34%] testing...... [35%] testing...... [36%] testing...... [37%] testing...... [38%] testing...... [39%] testing...... [40%] testing...... [41%] testing...... [42%] testing...... [43%] testing...... [44%] testing...... [45%] testing...... [46%] testing...... [47%] testing...... [48%] testing...... [49%] testing...... [50%] testing...... [51%] testing...... [52%] testing...... [53%] testing...... [54%] testing...... [55%] testing...... [56%] testing...... [57%] testing...... [58%] testing...... [59%] testing...... [60%] testing...... [61%] testing...... [62%] testing...... [63%] testing...... [64%] testing...... [65%] testing...... [66%] testing...... [67%] testing...... [68%] testing...... [69%] testing...... [70%] testing...... [71%] testing...... [72%] testing...... [73%] testing...... [74%] testing...... [75%] testing...... [76%] testing...... [77%] testing...... [78%] testing...... [79%] testing...... [80%] testing...... [81%] testing...... [82%] testing...... [83%] testing...... [84%] testing...... [85%] testing...... [86%] testing...... [87%] testing...... [88%] testing...... [89%] testing...... [90%] testing...... [91%] testing...... [92%] testing...... [93%] testing...... [94%] testing...... [95%] testing...... [96%] testing...... [97%] testing...... [98%] testing...... [99%] testing...... [100%]testing...... [100%] done!

making seq Get bench!
 testing...... [0%] testing...... [1%] testing...... [2%] testing...... [3%] testing...... [4%] testing...... [5%] testing...... [6%] testing...... [7%] testing...... [8%] testing...... [9%] testing...... [10%] testing...... [11%] testing...... [12%] testing...... [13%] testing...... [14%] testing...... [15%] testing...... [16%] testing...... [17%] testing...... [18%] testing...... [19%] testing...... [20%] testing...... [21%] testing...... [22%] testing...... [23%] testing...... [24%] testing...... [25%] testing...... [26%] testing...... [27%] testing...... [28%] testing...... [29%] testing...... [30%] testing...... [31%] testing...... [32%] testing...... [33%] testing...... [34%] testing...... [35%] testing...... [36%] testing...... [37%] testing...... [38%] testing...... [39%] testing...... [40%] testing...... [41%] testing...... [42%] testing...... [43%] testing...... [44%] testing...... [45%] testing...... [46%] testing...... [47%] testing...... [48%] testing...... [49%] testing...... [50%] testing...... [51%] testing...... [52%] testing...... [53%] testing...... [54%] testing...... [55%] testing...... [56%] testing...... [57%] testing...... [58%] testing...... [59%] testing...... [60%] testing...... [61%] testing...... [62%] testing...... [63%] testing...... [64%] testing...... [65%] testing...... [66%] testing...... [67%] testing...... [68%] testing...... [69%] testing...... [70%] testing...... [71%] testing...... [72%] testing...... [73%] testing...... [74%] testing...... [75%] testing...... [76%] testing...... [77%] testing...... [78%] testing...... [79%] testing...... [80%] testing...... [81%] testing...... [82%] testing...... [83%] testing...... [84%] testing...... [85%] testing...... [86%] testing...... [87%] testing...... [88%] testing...... [89%] testing...... [90%] testing...... [91%] testing...... [92%] testing...... [93%] testing...... [94%] testing...... [95%] testing...... [96%] testing...... [97%] testing...... [98%] testing...... [99%] testing...... [100%]testing...... [100%] done!

dpne!



[cdf]read---
--------------------------------------------
|            bench type:                   |
--------------------------------------------
----algorithm----
[0~100(usec)]: 131092
[100~200(usec)]: 0
[200~300(usec)]: 0
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----lower----
[0~100(usec)]: 131092
[100~200(usec)]: 0
[200~300(usec)]: 0
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----average----
[avg_algo]: 0.0
[avg_low]: 0.0
-----lower_info----
[write_op]: 442388
[read_op]: 327660
[trim_op]:19
[WAF, RAF]: 3.374638, 2.499466
[if rw test]: 6.749275(WAF), 4.998932(RAF)
[all write Time]:0.62987
[all read Time]:0.28862

----summary----
[all_time]: 2.842286
[size]: 1024.156250(mb)
[FAIL NUM] 0
[SUCCESS RATIO] 1.000000
[throughput] 368976.239548(kb/s)
             360.328359(mb/s)
[READ WRITE CNT] 0 131092



[cdf]read---
0		90107		0.687357
1		4706		0.723255
2		3089		0.746819
3		2037		0.762358
4		1726		0.775524
5		1407		0.786257
6		904		0.793153
7		436		0.796479
8		671		0.801597
9		502		0.805427
10		420		0.808631
11		410		0.811758
12		414		0.814916
13		302		0.817220
14		311		0.819592
15		498		0.823391
16		330		0.825909
17		130		0.826900
18		408		0.830013
19		4044		0.860861
20		3717		0.889215
21		5333		0.929897
22		3198		0.954292
23		1735		0.967527
24		1027		0.975361
25		878		0.982058
26		426		0.985308
27		424		0.988542
28		477		0.992181
635		2		0.992196
636		92		0.992898
637		121		0.993821
638		73		0.994378
639		76		0.994958
640		105		0.995759
641		84		0.996399
642		72		0.996949
643		79		0.997551
644		102		0.998329
645		90		0.999016
646		77		0.999603
647		52		1.000000
--------------------------------------------
|            bench type:                   |
--------------------------------------------
----algorithm----
[0~100(usec)]: 131091
[100~200(usec)]: 0
[200~300(usec)]: 1
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----lower----
[0~100(usec)]: 131092
[100~200(usec)]: 0
[200~300(usec)]: 0
[300~400(usec)]: 0
[400~500(usec)]: 0
[500~600(usec)]: 0
[600~700(usec)]: 0
[700~800(usec)]: 0
[800~900(usec)]: 0
[900~1000(usec)]: 0
[over_ms]: 0
[over__s]: 0
----average----
[avg_algo]: 0.0
[avg_low]: 0.0
-----lower_info----
[write_op]: 16364
[read_op]: 131092
[trim_op]:1
[WAF, RAF]: 0.124828, 1.000000
[if rw test]: 0.249657(WAF), 2.000000(RAF)
[all write Time]:0.1163
[all read Time]:0.33315

----summary----
[all_time]: 0.383225
[size]: 1024.156250(mb)
[FAIL NUM] 0
[SUCCESS RATIO] 1.000000
[throughput] 2736606.432253(kb/s)
             2672.467219(mb/s)
[READ WRITE CNT] 131092 0
result of ms:
bye bye!
---
--------------------------settings.h-----------------------------
#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>
#define ASYNC 1

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

#ifdef MLC

#define TOTALSIZE (300L*G)
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS ((_PPS)/_PPB)

#elif defined(SLC)

#define TOTALSIZE (10L*G)
//#define REALSIZE (512L*G)
//#define TOTALSIZE (128*M) // 1 Segment(16384 Page) 하나마다 TOTALSIZE 128 M만큼 필요
#define REALSIZE (256L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS (64)

#endif

#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOP (TOTALSIZE/PAGESIZE)
#define _NOS (TOTALSIZE/(_PPS*PAGESIZE))
#define _NOB (BPS*_NOS)
#define _RNOS (REALSIZE/(_PPS*PAGESIZE))//real number of segment
//#define RANGE (1*128*1024L) // 1 G
#define RANGE (10*128*1024L) // 32 M

#define FSTYPE uint8_t
#define KEYT uint32_t
#define BLOCKT uint32_t
#define OOBT uint64_t
#define V_PTR char * const
#define PTR char*
#define QSIZE (1024)
#define THREADSIZE (1)

#ifndef __GNUG__
typedef enum{false,true} bool;
#endif

typedef enum{
	SEQGET,SEQSET,
	RANDGET,RANDSET,
	RANDRW,SEQRW,
	MIXED
}bench_type;
#endif
----------------------------main.c-------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "../include/lsm_settings.h"
#include "../include/FS.h"
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "interface.h"
extern int req_cnt_test;
extern uint64_t dm_intr_cnt;
int main(){/*
>>>>>>> 5c776eb8c03af5769e5d04f2343aebeb491e8220
	int Input_cycle;
	int Input_type;
	int start;
	int end;
	int Input_size;
	printf("How many times would you run a benchmark?");
	scanf("%d", &Input_cycle);
	bench_init(Input_cycle);
	printf("please type the bench_type, start, end and input size\n");
	printf("====bench type list====\n");
	printf("SEQSET = 1, \nSEQGET = 2, \nRANDGET = 3, \nRANDRW = 4, \n");
	printf("SEQRW = 5, \nRANDSET = 6, \nMIXED = 7\n");
	printf("====bench type list end ====\n");
   printf("ex. 1 0 100 100 means seqset 0 to 100 with input size 100\n");
	for (int i = 0; i < Input_cycle; i++)
	{
		scanf("%d %d %d %d",&Input_type, &start, &end, &Input_size);
		if(Input_type == 1)
			bench_add(SEQSET,start,end,Input_size);
		else if(Input_type == 2)
			bench_add(SEQGET,start,end,Input_size);
		else if(Input_type == 3)
			bench_add(RANDGET,start,end,Input_size);
		else if(Input_type == 4)
			bench_add(RANDRW,start,end,Input_size);
		else if(Input_type == 5)
			bench_add(SEQRW,start,end,Input_size);
		else if(Input_type == 6)
			/bench_add(RANDSET,start,end,Input_size);
		else if(Input_type == 7)
			bench_add(MIXED,start,end,Input_size);
		else{
			printf("invalid setting input is detected. please rerun the bench!\n");
			return 0;
		}
		printf("benchmark # %d is initiailized.\n",i+1);
		
		if( i == Input_cycle -1)
			printf("initilization done.\n");
		else
			printf("please type in next benchmark settings.\n");

	}

	printf("benchmark setting done. starts now.\n");
*/

	inf_init();
#define RAND
#ifdef SEQ
	bench_init(2);
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	int a=128L*K, b=128L*K;
	bench_add(SEQSET,0,a,a+20);
	bench_add(SEQGET,0,a,a+20);
	//bench_add(SEQSET,0,1L*M,2L*M);
	//bench_add(SEQGET,0,1L*M,2L*M);
	//bench_add(RANDSET,0,1*1024,1*1024);
	//bench_add(RANDGET,0,1*1024,1*1024);
	//bench_add(RANDRW,0,16*1024,64*1024);
#endif
#ifdef RAND
	bench_init(1);
	char t_value[PAGESIZE];
	memset(t_value,'x',PAGESIZE);
	//bench_add(SEQSET,0,1*10,1*10);
	//bench_add(SEQGET,0,1*10,1*10);
	//bench_add(SEQSET,0,16*1024,32*1024);
	//bench_add(SEQGET,0,16*1024,32*1024);
	//bench_add(RANDSET,0,1*1024,1*1024);
	//bench_add(RANDGET,0,1*1024,1*1024);
	//bench_add(RANDRW,0,16*1024,32*1024);
	bench_add(RANDRW,0,200*RANGE,RANGE);
#endif

	/*
	for(int i=0; i<PAGESIZE;i++){
		t_value2[i]=rand()%256;
	}*/
//	bench_add(RANDSET,0,15*1024,15*1024);
//	bench_add(RANDGET,0,15*1024,15*1024);
	bench_value *value;

	value_set temp;
	temp.value=t_value;
	//temp.value=NULL;
	temp.dmatag=-1;
	temp.length=0;
	int cnt=0;
	while((value=get_bench())){
		temp.length=value->length;
		inf_make_req(value->type,value->key,&temp,value->mark);
		cnt++;
	}

	if(req_cnt_test==cnt){
		printf("dpne!\n");
	}
	else{
		printf("req_cnt_test:cnt -> %d:%d fuck\n",req_cnt_test,cnt);
	}

	while(!bench_is_finish()){
#ifdef LEAKCHECK
		sleep(1);
#endif
	}
	bench_print();

	bench_free();
	inf_free();
	return 0;
}
-----------------------simaultor result-------------------------------
mutex_t : 0x1809068 q:0x1809060, size:1024
!!! posix memory ASYNC: 1!!!
mutex_t : 0x180a6f8 q:0x180a6f0, size:1024
block_create start!
BM_Init() End!
block_create end!
TOTALSIZE: 10737418240
REALSIZE: 274877906944
PAGESIZE: 8192
_PPB: 256, ppb = 16384, _PPS: 16384, BPS: 64
BLOCKSIZE: 2097152
_NOP: 1310720
_NOS: 80
_NOB: 5120
_RNOS: 2048
RANGE: 1310720
async: 1
_nos:80
making rand Set and Get bench!
 testing...... [0%]