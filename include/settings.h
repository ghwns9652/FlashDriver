#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

#define TOTALSIZE (6L*G/8)
#define PAGESIZE (8*K)
//#define _PPB (16384) // After master merge, _PPB becomes 512. superblock: 2^14=16384
#define _PPB (256) // After master merge, _PPB becomes 512. superblock: 2^14=16384
#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOB (TOTALSIZE/BLOCKSIZE)
#define _NOP (TOTALSIZE/PAGESIZE)

#define SEGNUM (TOTALSIZE/((1<<14)*PAGESIZE))
#define BPS ((1<<14)/_PPB)

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
