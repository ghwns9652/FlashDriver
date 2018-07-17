#ifndef __H_SETTING__
#define __H_SETTING__
#include<stdint.h>
#define ASYNC 1

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)

<<<<<<< HEAD
#define TOTALSIZE (16L*G)
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
//#define _PPB (16384) // After master merge, _PPB becomes 512. superblock: 2^14=16384
#define _PPB (256) // After master merge, _PPB becomes 512. superblock: 2^14=16384
//#define _PPB (64) // After master merge, _PPB becomes 512. superblock: 2^14=16384
#define _PPS (1<<14)
=======
#ifdef MLC

#define TOTALSIZE (300L*G)
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS ((_PPS)/_PPB)

#elif defined(SLC)

#define TOTALSIZE (200L*G)
#define REALSIZE (512L*G)
#define PAGESIZE (8*K)
#define _PPB (256)
#define _PPS (1<<14)
#define BPS (64)

#endif

>>>>>>> 55127f0306b75cd29328f82e06393fec173a8f9d
#define BLOCKSIZE (_PPB*PAGESIZE)
#define _NOP (TOTALSIZE/PAGESIZE)
#define _NOS (TOTALSIZE/(_PPS*PAGESIZE))
<<<<<<< HEAD
#define _RNOS (REALSIZE/(_PPS*PAGESIZE))//real number of segment

#define BPS ((_PPS)/_PPB)
=======
#define _NOB (BPS*_NOS)
#define _RNOS (REALSIZE/(_PPS*PAGESIZE))//real number of segment
#define RANGE (25*128*1024L)

>>>>>>> 55127f0306b75cd29328f82e06393fec173a8f9d

#define FSTYPE uint8_t
#define KEYT uint32_t
#define BLOCKT uint32_t
#define OOBT uint64_t
#define V_PTR char * const
#define PTR char*
<<<<<<< HEAD
=======
#define ASYNC 1
>>>>>>> 55127f0306b75cd29328f82e06393fec173a8f9d
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
