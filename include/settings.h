#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

#define TOTALSIZE (30L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
//#define _PPB (4)
#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOB (TOTALSIZE/BLOCKSIZE)
//#define _NOB (4)
//#define _NOP (_PPB*_NOB)
#define _NOP (TOTALSIZE/PAGESIZE)
//#define TOTALSIZE (PAGESIZE*_NOP)

#define FSTYPE uint8_t
#define KEYT uint32_t
#define BLOCKT uint32_t
#define OOBT uint64_t
#define V_PTR char * const
#define PTR char*
#define ASYNC 1
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
