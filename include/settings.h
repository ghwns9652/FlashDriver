#ifndef __H_SETTING__
#define __H_SETTING__
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#include<stdint.h>
#include <stdlib.h>
#include<stdio.h>
#include <string.h>

/*
#define free(a) \
	do{\
		printf("%s %d:%p\n",__FILE__,__LINE__,a);\
		free(a)\
	}while(0)
*/

#define LOWER_FILE_NAME "./data/simulator.data"

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

#define GIGAUNIT 8L
#define TOTALSIZE ((16L)*G)
#define REALSIZE (512L*G)
#define DEVSIZE (64L * G)
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

#define RANGE ((GIGAUNIT)*(M/PAGESIZE)*1024L*0.8)
//#define RANGE ((GIGAUNIT)*(M/PAGESIZE)*1024L*0.8)
//#define RANGE (50*(M/PAGESIZE)*1024L*0.8)

#define SIMULATION 0

#define FSTYPE uint8_t
#ifdef KVSSD
#define KEYFORMAT(input) input.len,input.key
#include<string.h>
typedef struct str_key{
	uint8_t len;
	char *key;
}str_key;

	#define KEYT str_key
static inline int KEYCMP(KEYT a,KEYT b){
	if(!a.len && !b.len) return 0;
	else if(a.len==0) return -1;
	else if(b.len==0) return 1;

	int r=memcmp(a.key,b.key,a.len>b.len?b.len:a.len);
	if(r!=0 || a.len==b.len){
		return r;
	}
	return a.len<b.len?-1:1;
}

static inline int KEYCONSTCOMP(KEYT a, char *s){
	int len=strlen(s);
	if(!a.len && !len) return 0;
	else if(a.len==0) return -1;
	else if(len==0) return 1;

	int r=memcmp(a.key,s,a.len>len?len:a.len);
	if(r!=0 || a.len==len){
		return r;
	}
	return a.len<len?-1:1;
}
#define KEYLEN(a) (a.len+sizeof(uint32_t))
#else
	#define KEYT uint32_t
#endif
#define BLOCKT uint32_t
#define OOBT uint64_t
#define V_PTR char * const
#define PTR char*
#define ASYNC 1
#define QSIZE (1024)
#define QDEPTH (128)
#define THREADSIZE (1)

#define THPOOL
#define NUM_THREAD 64

#define TCP 1
//#define IP "10.42.0.2"
//#define IP "127.0.0.1"
//#define IP "10.42.0.1"
#define IP "192.168.0.1"
#define PORT 9999
#define NETWORKSET
#define DATATRANS

#define KEYGEN
#define SPINSYNC
#define interface_pq

#ifndef __GNUG__
typedef enum{false,true} bool;
#endif

#endif
