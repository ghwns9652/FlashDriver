#ifndef _BM_BLOCK_H_
#define _BM_BLOCK_H_

#include "BM_common.h"


typedef struct {
	uint32_t	PBA;			/* PBA of this block */
	int8_t		ValidP[NOP];	/* index means ValidPity of offset pages */
	uint8_t		numValid;		/* Number of inValidPate pages in this block*/
	uint32_t	PE_cycle;		/* P/E cycles of this block */
	uint8_t		*ptrNV_data;	/* Pointer of numValid map */	//Heap이나 Quicksort에서 SWAP할 때 이것도 새로 맞춰야 할 것 같은데.. 애초에 이걸 어디 쓸지 잘 모르겠다
	uint32_t	*ptrPE_data;	/* Pointer of PE map */
	int8_t		BAD;			/* Whether this block is bad or not */
} Block;

/* Type of member variable */
#if 0
#define PBA_T	uint32_t
#define ValidP_T	int8_t /* Caution: ValidP type is actually ARRAY of int8_t */
#define nV_T	uint8_t
#define PE_T	uint32_t
#endif // 0
typedef uint32_t	PBA_T;
typedef int8_t		ValidP_T;  /* Caution: ValidP type is actually ARRAY of int8_t */
typedef uint8_t		nV_T;
typedef uint32_t	PE_T;
typedef int8_t		Bad_T;

/* 이름들 좀 일관적으로 바꾸자...  nV_map, PE_map 혹은 numValid_map, PE_map 이렇게 */
uint8_t* numValid_map[NOB];	/* Array of pointers to numValid */
uint32_t* PE_map[NOB];		/* Array of pointers to PE */

//Block blockArray[NOB];	/* Array of Blocks */

/* PPA를 가지고 PBA를 찾는 매크로 */
#define BM_PPA_TO_PBA(PPA)	PPA/PPB

/* ValidP member variable에서 해당 page가 ValidP한지를 나타내는 macro */
#define BM_VALIDPAGE	1
#define BM_INVALIDPAGE	0
#define BM_WRITTEN		1	/* Not Determined yet */

int32_t BM_Find_BlockPlace_by_PPA(Block* ptrBlock[], uint32_t size, uint32_t PPA);
int32_t BM_search_PBA(Block* ptrBlock[], uint32_t size, uint32_t PBA);



/* Common useful functions */
void print_arr(int32_t *arr, uint32_t size);
void print_arr2(Block* arr[], uint32_t size);
void print_arr_ValidP(Block* arr[], uint32_t size); // Can't work with correct manner
void print_arr_cnt(Block* arr[], uint32_t size);
void print_arr_PE(Block* arr[], uint32_t size);
void print_arr_all(Block* arr[], uint32_t size);



#endif // !_BM_BLOCK_H_


