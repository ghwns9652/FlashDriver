#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

<<<<<<< HEAD
#define TOTALSIZE (1L*G)
=======
#define TOTALSIZE (16L*G)
>>>>>>> 57005bd88a70613230a91efedc8098b375ad7437
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
#define _PPB (1<<14)
#define _PPS (1<<14)
#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOB (TOTALSIZE/BLOCKSIZE)
#define _NOP (TOTALSIZE/PAGESIZE)
#define _NOS (TOTALSIZE/(_PPS*PAGESIZE))
#define _RNOS (REALSIZE/(_PPS*PAGESIZE))//real number of segment

//#define PAGESIZE (8*K)
//#define _PPB (256)
//#define BLOCKSIZE (PAGESIZE*_PPB)
//#define _NOB (BLOCKSIZE*)
//#define _NOP (_PPB*_NOB)
//#define TOTALSIZE (PAGESIZE * _NOP)
#define BPS ((_PPS)/_PPB)

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
