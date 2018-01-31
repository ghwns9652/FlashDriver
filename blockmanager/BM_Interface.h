#ifndef _BM_INTERFACE_H_
#define _BM_INTERFACE_H_

#include "BM_common.h"
#include "BM_Block.h"
#include "BM_Heap.h"


/* parameter로 받은 PPA에 해당하는 Page의 상태를 INVALID로 바꾸는 함수 */
int32_t		BM_invalidate_ppa(Block* blockArray, uint32_t PPA);


/* parameter로 받은 PPA가 valid인지 invalid인지 확인하여 반환하는 함수*/
int32_t		BM_is_invalid_ppa(Block* blockArray, uint32_t PPA);


/*
 * GC victim block의 PBA를 반환하는 함수
 * numValid_map에는 numValid(각 block당 valid page의 개수)에 대한 포인터들이 들어있으며, numValid에 따라 Max-heap으로 정렬될 것
 * numValid가 가장 큰 block을 GC victim으로 선정하여 PBA를 반환함
 */
uint32_t	BM_get_gc_victim(Block* blockArray, uint8_t* numValid_map[]);



/* 아직 구체적으로 설계하지 못하여 구현되지 않은 상태
    PE_cycle이 가장 낮은 block을 return하여 free block pool에 할당하거나 PE_cycle 순으로 정렬해놓는 등의 방식이 될 것 같음
    경택이와 상의할 필요가 있을 듯함
	*/
uint32_t	BM_get_worn_block(Block *blockArray, uint32_t* PE_map[]);



/* GC할 때마다 불러야 하는 함수*/


#endif // !_BM_INTERFACE_H_
