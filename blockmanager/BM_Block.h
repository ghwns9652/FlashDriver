#ifndef _BM_BLOCK_H_
#define _BM_BLOCK_H_

#include "BM_common.h"


typedef struct { // 40 bytes
	uint32_t	PBA;			/* PBA of this block */
	uint64_t	ValidP[4];		/* index means Validity of offset pages. 1 means VALID, 0 means INVALID */
	int16_t		numValid;		/* Number of Valid pages in this block*/
	uint32_t	PE_cycle;		/* P/E cycles of this block */
	int16_t**	ptrNV_data;		/* Pointer of numValid map */
	uint32_t**	ptrPE_data;		/* Pointer of PE map */
	int8_t		BAD;			/* Whether this block is bad or not */
	uint32_t	v_PBA;			/* virtual PBA of this block. It is virtual block after Bad-Block check and wear-leveling by PE_cycle */
	uint32_t	o_PBA;			/* original PBA of this block. We can find the PBA(index) of blockArray with v_PBA using o_PBA */
} Block;


/* Type of member variable */
typedef uint32_t	PBA_T;
typedef uint64_t	ValidP_T;  /* Caution: ValidP type is actually ARRAY of uint64_t */
typedef int16_t		nV_T;
typedef uint32_t	PE_T;
typedef int8_t		BAD_T;

typedef uint32_t	PPA_T;

/* BAD Status of blocks */
#define _BADSTATE	1
#define _NOTBADSTATE	0

/* (IGNORE!) Don't Care about that NOW (It's incomplete) */
/* Whether blockArray exists or not */
#define _BA_EXIST	32
#define _BA_GOODSTATE	10
#define _BA_BADSTATE	20

/* Macros for finding PBA from PPA */
#define BM_PPA_TO_PBA(PPA)	PPA/_PPB

/* Macros that indicate whether the page is valid or not */
#define BM_VALIDPAGE	1
#define BM_INVALIDPAGE	0
#define BM_WRITTEN		1	/* (IGNORE!) Not Determined yet */

/* Macros for finding member variables from Block ptr */
#define BM_GETPBA(ptr_block)		((Block*)ptr_block)->PBA
#define BM_GETVALIDP(ptr_block)		((Block*)ptr_block)->ValidP
#define BM_GETNUMVALID(ptr_block)	((Block*)ptr_block)->numValid
#define BM_GETPECYCLE(ptr_block)	((Block*)ptr_block)->PE_cycle
#define BM_GETBAD(ptr_block)		((Block*)ptr_block)->BAD
#define BM_GETVPBA(ptr_block)		((Block*)ptr_block)->v_PBA
#define BM_GETOPBA(ptr_block)		((Block*)ptr_block)->o_PBA



#if 0
#define __GETVALIDP	sizeof(PBA_T)
#define __GETNUMVALID	sizeof(PBA_T) + sizeof(ValidP_T)*_NOP
#define __GETPECYCLE	sizeof(PBA_T) + sizeof(ValidP_T)*_NOP + sizeof(nV_T)
#define __GETBAD		sizeof(PBA_T) + sizeof(ValidP_T)*_NOP + sizeof(nV_T) + sizeof(PE_T) + sizeof(uint8_t**) + sizeof(uint32_t**)
#define __GETVPBA		sizeof(PBA_T) + sizeof(ValidP_T)*_NOP + sizeof(nV_T) + sizeof(PE_T) + sizeof(uint8_t**) + sizeof(uint32_t**) + sizeof(BAD_T)
#define __GETOPBA		sizeof(PBA_T) + sizeof(ValidP_T)*_NOP + sizeof(nV_T) + sizeof(PE_T) + sizeof(uint8_t**) + sizeof(uint32_t**) + sizeof(BAD_T) + sizeof(PBA_T)

#define BM_GETPBA(ptr_block)		*((PBA_T*)(ptr_block))
#define BM_GETVALIDP(ptr_block)		*((ValidP_T*)(ptr_block + __GETVALIDP))
#define BM_GETNUMVALID(ptr_block)	*((nV_T*)(ptr_block + __GETNUMVALID))
#define BM_GETPECYCLE(ptr_block)	*((PE_T*)(ptr_block + __GETPECYCLE))
#define BM_GETBAD(ptr_block)		*((BAD_T*)(ptr_block + __GETBAD))
#define BM_GETVPBA(ptr_block)		*((PBA_T*)(ptr_block + __GETVPBA))
#define BM_GETOPBA(ptr_block)		*((PBA_T*)(ptr_block + __GETOPBA))
#endif



int32_t BM_Init();

int32_t BM_InitBlock();

int32_t BM_LoadBlock(uint32_t PBA_BM); 

int32_t BM_InitBlockArray();

int32_t BM_ScanFlash();

int32_t BM_ReadBlock(int i);

int32_t BM_BadBlockCheck();

int32_t BM_FillMap();

int32_t BM_MakeVirtualPBA();

int32_t BM_Shutdown();





#endif // !_BM_BLOCK_H_


