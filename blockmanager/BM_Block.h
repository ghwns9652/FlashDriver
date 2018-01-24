#ifndef _BM_BLOCK_H_
#define _BM_BLOCK_H_

#include "BM_common.h"

struct Block_ {
	uint32_t PBA;		/* PBA of this block */
	uint64_t bit[4];	/* index means Validity of offset pages */
	uint8_t cnt;		/* Number of invalidate pages in this block*/
	uint32_t PE_cycle;	/* P/E cycles of this block */
};


// Block: 64bit짜리 4개로 각 page offset 별 invalid 여부를 표시하자.
typedef struct Block_ Block;

/* PPA를 가지고 PBA를 찾는 매크로 */
#define BM_PBA_TO_PPA(PPA)	PPA/PPB

int32_t BM_Find_BlockPlace_by_PPA(Block* ptrBlock[], uint32_t size, uint32_t PPA);
int32_t BM_search_PBA(Block* ptrBlock[], uint32_t size, uint32_t PBA);



/* Common useful functions */
void print_arr(int32_t *arr, uint32_t size);
void print_arr2(Block* arr[], uint32_t size);
void print_arr_bit(Block* arr[], uint32_t size);
void print_arr_cnt(Block* arr[], uint32_t size);
void print_arr_PE(Block* arr[], uint32_t size);
void print_arr_all(Block* arr[], uint32_t size);



#endif // !_BM_BLOCK_H_


