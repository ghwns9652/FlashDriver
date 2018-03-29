#ifndef _BM_BLOCK_H_
#define _BM_BLOCK_H_

#include "BM_common.h"


typedef struct { // 24 bytes
	uint32_t	PBA;			/* PBA of this block */
	int8_t		ValidP[_NOP];	/* index means Validity of offset pages. 1 means VALID, 0 means INVALID */
	uint8_t		numValid;		/* Number of Valid pages in this block*/
	uint32_t	PE_cycle;		/* P/E cycles of this block */
	uint8_t**	ptrNV_data;		/* Pointer of numValid map */
	uint32_t**	ptrPE_data;		/* Pointer of PE map */
	int8_t		BAD;			/* Whether this block is bad or not */
	uint32_t	v_PBA;			/* virtual PBA of this block. It is virtual block after Bad-Block check and wear-leveling by PE_cycle */
	uint32_t	o_PBA;			/* original PBA of this block. We can find the PBA(index) of blockArray with v_PBA using o_PBA */
} Block;

/* Type of member variable */
typedef uint32_t	PBA_T;
typedef int8_t		ValidP_T;  /* Caution: ValidP type is actually ARRAY of int8_t */
typedef uint8_t		nV_T;
typedef uint32_t	PE_T;
typedef int8_t		Bad_T;


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


int32_t BM_Find_BlockPlace_by_PPA(Block* ptrBlock[], uint32_t size, uint32_t PPA);
int32_t BM_search_PBA(Block* ptrBlock[], uint32_t size, uint32_t PBA);




int32_t BM_Init();

int32_t BM_InitBlock();

int32_t BM_LoadBlock(uint32_t PBA_BM); 

int32_t BM_InitBlockArray();

int32_t BM_ScanFlash();

int32_t BM_ReadBlock();

int32_t BM_BadBlockCheck();

int32_t BM_FillMap();

int32_t BM_Shutdown();





#endif // !_BM_BLOCK_H_


